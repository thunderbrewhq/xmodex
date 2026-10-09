#include "xmodex/output/Output.hpp"
#include "xmodex/LocalCriticalSection.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelPool.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/dsp/DSPResampler.hpp"
#include "xmodex/os/Time.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

Output::Output() {
    this->mPolling = false;
    this->mChannelPool = nullptr;
    this->mChannelPool3D = nullptr;
    this->mRecordLoop = false;
    this->mMusicChannelGroup = nullptr;
    this->mNum2DChannelsFromCaps = 0;
    this->mNum3DChannelsFromCaps = 0;
    this->mTotalChannelsFromCaps = 0;
}

FMOD_RESULT Output::release() {
    FMOD_Memory_Free(this);
    return FMOD_OK;
}

FMOD_RESULT Output::getFreeChannel(FMOD_MODE mode, ChannelReal** realchannel, int32_t numchannels, int32_t numsoundchannels, int32_t* found) {
    FMOD_RESULT result;

    if (!realchannel) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (mode & FMOD_3D) {
        if (!this->mChannelPool3D) {
            return FMOD_ERR_CHANNEL_ALLOC;
        }

        result = this->mChannelPool3D->allocateChannel(realchannel, FMOD_CHANNEL_FREE, numchannels, found);
        if (result != FMOD_OK) {
            return result;
        }
    } else {
        if (!this->mChannelPool) {
            return FMOD_ERR_CHANNEL_ALLOC;
        }

        result = this->mChannelPool->allocateChannel(realchannel, FMOD_CHANNEL_FREE, numchannels, found);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT Output::recordRead(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    FMOD_RESULT result = FMOD_OK;
    char* rptr1 = nullptr;
    char* rptr2 = nullptr;
    auto destptr = outbuffer;
    uint32_t rlen1 = 0;
    uint32_t rlen2 = 0;
    uint32_t roffsetbytes = 0;
    uint32_t rlenbytes = 0;
    uint32_t rblockalign = 0;

    SoundI::getBytesFromSamples(this->mRecordLastCursorPos, &roffsetbytes, outchannels, this->mRecordFormat);
    SoundI::getBytesFromSamples(length, &rlenbytes, outchannels, this->mRecordFormat);
    SoundI::getBytesFromSamples(1, &rblockalign, outchannels, this->mRecordFormat);

    result = this->mDescription.record_lock(this, roffsetbytes, rlenbytes, reinterpret_cast<void**>(&rptr1), reinterpret_cast<void**>(&rptr2), &rlen1, &rlen2);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mRecordFormat == FMOD_SOUND_FORMAT_PCM8) {
        uint8_t* rptr = nullptr;

        if (rptr1 && rlen1) {
            rptr = reinterpret_cast<uint8_t*>(rptr1);
            for (uint32_t count = 0; count < rlen1; count++) {
                *rptr++ ^= 128;
            }
        }

        if (rptr2 && rlen2) {
            rptr = reinterpret_cast<uint8_t*>(rptr2);
            for (uint32_t count = 0; count < rlen2; count++) {
                *rptr++ ^= 128;
            }
        }
    }

    if (rptr1 && rlen1) {
        DSPI::convert(destptr, rptr1, FMOD_SOUND_FORMAT_PCMFLOAT, this->mRecordFormat, (rlen1 / rblockalign) * outchannels, 1, 1, 1.0f);
        destptr += ((rlen1 / rblockalign) * outchannels);
    }

    if (rptr2 && rlen2) {
        DSPI::convert(destptr, rptr2, FMOD_SOUND_FORMAT_PCMFLOAT, this->mRecordFormat, (rlen2 / rblockalign) * outchannels, 1, 1, 1.0f);
    }

    if (this->mDescription.record_unlock) {
        result = this->mDescription.record_unlock(this, rptr1, rptr2, rlen1, rlen2);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mRecordLastCursorPos += length;
    if (this->mRecordLastCursorPos >= this->mRecordBufferLength) {
        this->mRecordLastCursorPos -= this->mRecordBufferLength;
    }

    return FMOD_OK;
}

FMOD_RESULT Output::recordFill(uint32_t length) {
    FMOD_RESULT result;
    FMOD_SOUND_FORMAT pformat;
    char* pptr1 = nullptr;
    char* pptr2 = nullptr;
    int32_t channels;
    uint32_t pblockalign;
    uint32_t plen1 = 0;
    uint32_t plen2 = 0;
    uint32_t plength;
    uint32_t plengthbytes;
    uint32_t poffsetbytes;
    uint32_t soundlength;
    uint32_t totalplength;

    result = this->mRecordSound->getFormat(nullptr, &pformat, &channels, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mRecordSound->getLength(&soundlength, FMOD_TIMEUNIT_PCM);
    if (result != FMOD_OK) {
        return result;
    }

    result = SoundI::getBytesFromSamples(1, &pblockalign, channels, pformat);
    if (result != FMOD_OK) {
        return result;
    }

    totalplength = static_cast<uint32_t>((length * this->mRecordSound->mDefaultFrequency) / this->mRecordRate);
    while (totalplength) {
        plength = totalplength;
        if (plength > this->mRecordTempBufferLength) {
            plength = this->mRecordTempBufferLength;
        }

        SoundI::getBytesFromSamples(this->mRecordOffset, &poffsetbytes, channels, pformat);
        SoundI::getBytesFromSamples(plength, &plengthbytes, channels, pformat);

        if (this->mRecordResample) {
            result = this->mRecordResamplerDSP.execute(this->mRecordTempBuffer, &this->mRecordTempBuffer, &plength, channels, &channels, static_cast<FMOD_SPEAKERMODE>(0));
            if (result != FMOD_OK) {
                return result;
            }
        } else {
            result = this->recordRead(this->mRecordTempBuffer, this->mRecordTempBuffer, plength, channels, channels);
            if (result != FMOD_OK) {
                return result;
            }
        }

        result = this->mRecordSound->lock(poffsetbytes, plengthbytes, reinterpret_cast<void**>(&pptr1), reinterpret_cast<void**>(&pptr2), &plen1, &plen2);
        if (result != FMOD_OK) {
            return result;
        }

        auto srcptr = this->mRecordTempBuffer;

        if (pptr1 && plen1) {
            DSPI::convert(pptr1, srcptr, pformat, FMOD_SOUND_FORMAT_PCMFLOAT, (plen1 / pblockalign) * channels, 1, 1, 1.0f);
            srcptr += ((plen1 / pblockalign) * channels);
        }
        if (pptr2 && plen2) {
            DSPI::convert(pptr2, srcptr, pformat, FMOD_SOUND_FORMAT_PCMFLOAT, (plen2 / pblockalign) * channels, 1, 1, 1.0f);
        }

        result = this->mRecordSound->unlock(pptr1, pptr2, plen1, plen2);
        if (result != FMOD_OK) {
            return result;
        }

        this->mRecordOffset += plength;

        if (plen2 || this->mRecordOffset >= static_cast<int32_t>(soundlength)) {
            if (this->mRecordLoop) {
                this->mRecordOffset -= soundlength;
                if (this->mRecordOffset < 0) {
                    this->mRecordOffset = 0;
                }
            } else {
                this->mSystem->recordStop();
                break;
            }
        }

        totalplength -= plength;
    }

    return FMOD_OK;
}

FMOD_RESULT Output::recordUpdate() {
    FMOD_RESULT result;
    uint32_t cursorpos;
    int32_t length;

    if (!this->mDescription.record_getposition) {
        return FMOD_OK;
    }

    if (!this->mRecordBufferLength) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->mDescription.record_getposition(this, &cursorpos);
    if (result != FMOD_OK) {
        return result;
    }

    length = cursorpos - this->mRecordLastCursorPos;
    if (length < 0) {
        length += this->mRecordBufferLength;
    }
    if (length < 0 || length > static_cast<int32_t>(this->mRecordBufferLength)) {
        length = 0;
    }

    if (this->mRecordResample) {
        auto blocklength = this->mRecordResamplerDSP.mDescription.mResamplerBlockLength;
        if (length < blocklength * 3) {
            return FMOD_OK;
        }

        length /= blocklength;
        length *= blocklength;
        length -= blocklength;
    } else {
        if (length <= 0) {
            return FMOD_OK;
        }
    }

    return this->recordFill(length);
}

FMOD_RESULT Output::mix(void* buffer, uint32_t numsamples) {
    FMOD_RESULT result;
    FMOD_SOUND_FORMAT outputformat;
    LocalCriticalSection criticalsection(this->mSystem->mDSPCrit);
    LocalCriticalSection lockdspcriticalsection(this->mSystem->mDSPLockCrit);
    int32_t outputchannels;
    uint32_t blockalign;
    DSPI* dsphead;
    uint32_t offset = 0;

    if (!buffer || !numsamples) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->mSystem->getSoftwareFormat(nullptr, &outputformat, &outputchannels, nullptr, nullptr, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = SoundI::getBytesFromSamples(1, &blockalign, outputchannels, outputformat);
    if (result != FMOD_OK) {
        return result;
    }

    dsphead = this->mSystem->mDSPSoundCard;
    if (!dsphead) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mSystem->flushDSPConnectionRequests(false);

    lockdspcriticalsection.enter();
    criticalsection.enter();

    if (this->mRecording) {
        this->recordUpdate();
    }

    do {
        void* outbuffer = reinterpret_cast<char*>(buffer) + (offset * blockalign);
        auto len = numsamples;

        this->mSystem->mDSPActive = true;
        this->mSystem->mDSPTempBuffIndex = 0;

        auto endbuffer = outbuffer;

        dsphead->execute(outbuffer, &endbuffer, &len, outputchannels, &outputchannels, this->mSystem->mSpeakerMode);

        if (endbuffer != outbuffer) {
            FMOD_memcpy(outbuffer, endbuffer, len * blockalign);
        }

        this->mSystem->mDSPActive = false;
        dsphead->resetVisited();

        numsamples -= len;
        offset += len;
    } while (numsamples);

    if (this->mDescription.postmixcallback) {
        this->mDescription.postmixcallback(this);
    }

    criticalsection.leave();
    lockdspcriticalsection.leave();

    auto delta = (static_cast<float>(this->mSystem->mDSPBlockSize) / static_cast<float>(this->mSystem->mOutputRate)) * 1000.0f;
    gGlobal->gDSPClock.mValue += static_cast<FMOD_UINT64>(delta * 4294967296.0f);
    FMOD_OS_Time_GetMs(&gGlobal->gDSPClockTimeStamp);

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK Output::recordResamplerReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto resampler = static_cast<DSPResampler*>(dsp_state);

    Output* output = nullptr;
    resampler->getUserData(reinterpret_cast<void**>(&output));

    return output->recordRead(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK Output::mixCallback(FMOD_OUTPUT_STATE* output, void* buffer, uint32_t length) {
    return static_cast<Output*>(output)->mix(buffer, length);
}

} // namespace FMOD
