#include "xmodex/channel/ChannelStream.hpp"
#include "xmodex/AsyncData.hpp"
#include "xmodex/LocalCriticalSection.hpp"
#include "xmodex/SpeakerMap.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelReal.hpp"
#include "xmodex/codec/Codec.hpp"
#include "xmodex/os/CriticalSection.hpp"
#include "xmodex/os/Thread.hpp"
#include "xmodex/sound/Sample.hpp"
#include "xmodex/sound/SoundI.hpp"
#include "xmodex/sound/Stream.hpp"

namespace FMOD {

ChannelStream::ChannelStream() {
    this->mSamplesWritten = 0;
    this->mSamplesPlayed = 0;
}

FMOD_RESULT ChannelStream::set2DFreqVolumePanFor3D() {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->set2DFreqVolumePanFor3D();
    }

    return result;
}

bool ChannelStream::isStream() {
    return true;
}

FMOD_RESULT ChannelStream::moveChannelGroup(ChannelGroupI* oldchannelgroup, ChannelGroupI* newchannelgroup) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->moveChannelGroup(oldchannelgroup, newchannelgroup);
    }

    return result;
}

FMOD_RESULT ChannelStream::alloc() {
    FMOD_RESULT result;

    auto stream = static_cast<Stream*>(this->mSound);

    this->mFlags &= ~CHANNELREAL_FLAG_STOPPED;

    this->mSystem = stream->mSystem;
    this->mFinished = false;
    this->mLastPCM = 0;
    this->mDecodeOffset = 0;

    if (stream->mSubSoundParent) {
        auto streamParent = static_cast<Stream*>(stream->mSubSoundParent);
        this->mPosition = streamParent->mInitialPosition;
    } else {
        this->mPosition = stream->mInitialPosition;
    }

    this->mSamplesPlayed = 0;
    this->mSamplesWritten = 0;
    this->mMinFrequency = this->mSubChannel[0]->mMinFrequency;
    if (this->mMinFrequency < 100.0f) {
        this->mMinFrequency = 100.0f;
    }
    this->mMaxFrequency = this->mSubChannel[0]->mMaxFrequency;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        auto sample = static_cast<Sample*>(stream->mSample);

        if (sample->mNumSubSamples) {
            sample = static_cast<Sample*>(sample->mSubSample[i]);
        }

        sample->mSubSoundParent = stream;
        this->mSubChannel[i]->mSound = sample;
        this->mSubChannel[i]->mSubChannelIndex = i;
        this->mSubChannel[i]->mDSP = nullptr;
        this->mSubChannel[i]->mLoopStart = sample->mLoopStart;
        this->mSubChannel[i]->mLoopLength = sample->mLoopLength;
        this->mSubChannel[i]->mLoopCount = -1;
        this->mSubChannel[i]->mDelayStart = 0;
        this->mSubChannel[i]->mDelayEnd = 0;
        this->mSubChannel[i]->mParent = this->mParent;

        result = this->mSubChannel[i]->alloc();
        if (result != FMOD_OK) {
            return result;
        }
    }

    FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamListCrit);
    this->mStreamNode.setData(this);
    this->mStreamNode.addBefore(&this->mSystem->mStreamListChannelHead);
    FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamListCrit);

    return FMOD_OK;
}

FMOD_RESULT ChannelStream::start() {
    FMOD_RESULT result;

    if (!this->mSubChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->start();
        if (result != FMOD_OK) {
            return result;
        }

        this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_STOPPED;
        this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ALLOCATED;
        this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ENDDELAY;
        this->mSubChannel[i]->mFlags |= CHANNELREAL_FLAG_PLAYING;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelStream::update(int32_t delta) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->update(delta);
    }

    return result;
}

FMOD_RESULT ChannelStream::updateStream() {
    FMOD_RESULT result = FMOD_OK;
    Sample* sample;
    Stream* stream;
    uint32_t pcm = 0;
    int32_t delta;
    LocalCriticalSection crit(this->mSystem->mStreamRealchanCrit);

    crit.enter();
    if (!this->mSound) {
        return FMOD_OK;
    }

    stream = static_cast<Stream*>(this->mSound);
    sample = stream->mSample;

    if (stream->mOpenState != FMOD_OPENSTATE_READY) {
        return FMOD_ERR_NOTREADY;
    }

    stream->mFlags |= FMOD_SOUND_FLAG_PLAYED;

    if (this->mFinished) {
        stream->mFlags |= FMOD_SOUND_FLAG_FINISHED;
        return FMOD_OK;
    }

    if (this->mSubChannel[0]) {
        result = this->mSubChannel[0]->updateStream();
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mSubChannel[0]->getPosition(&pcm, FMOD_TIMEUNIT_PCM);
        if (result != FMOD_OK) {
            return result;
        }
    }
    crit.leave();

    while ((this->mSamplesPlayed > this->mSamplesWritten && this->mSamplesPlayed - this->mSamplesWritten >= static_cast<uint32_t>(stream->mBlockSize)) ||
           (this->mSamplesPlayed && this->mSamplesPlayed < this->mSamplesWritten && this->mSamplesWritten - this->mSamplesPlayed >= static_cast<uint32_t>(stream->mBlockSize))) {
        uint32_t len;

        crit.enter();
        len = stream->mBlockSize;

        if (this->mDecodeOffset > sample->mLength) {
            len = 0;
        } else if (this->mDecodeOffset + len > sample->mLength) {
            len = sample->mLength - this->mDecodeOffset;
        }
        crit.leave();

        result = stream->fill(this->mDecodeOffset, len);

        crit.enter();
        if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
            for (int32_t i = 0; i < this->mNumSubChannels; i++) {
                if (this->mSubChannel[i]) {
                    this->mSubChannel[i]->setPaused(true);
                }
            }

            stream->mOpenState = FMOD_OPENSTATE_ERROR;
            if (stream->mAsyncData) {
                stream->mAsyncData->mResult = result;
            }

            this->mFinished = true;
            return result;
        }

        stream->mFlags |= FMOD_SOUND_FLAG_WANTSTOFLUSH;

        this->mDecodeOffset += len;
        if (this->mDecodeOffset >= sample->mLength) {
            this->mDecodeOffset -= sample->mLength;
        }
        this->mSamplesWritten += len;

        if (stream->mLength < this->mLoopStart + this->mLoopLength) {
            this->mLoopLength = stream->mLength - this->mLoopStart;
        }

        uint32_t endpoint;

        if (this->mMode & FMOD_LOOP_NORMAL && this->mLoopCount) {
            endpoint = this->mLoopStart + this->mLoopLength - 1;
        } else {
            endpoint = stream->mLength - 1;
        }

        if (endpoint < this->mPosition) {
            len = 0;
        } else if (endpoint < this->mPosition + len) {
            len = (endpoint - this->mPosition) + 1;
        }

        this->mPosition += len;

        if (this->mPosition > endpoint) {
            if ((this->mMode & FMOD_LOOP_NORMAL && this->mLoopCount) || stream->mLength == static_cast<uint32_t>(-1)) {
                this->mPosition = this->mLoopStart;

                if (this->mLoopCount > 0) {
                    this->mLoopCount--;
                }
            } else if (stream->mFlags & FMOD_SOUND_FLAG_FINISHED) {
                this->mPosition = stream->mLength;

                for (int32_t i = 0; i < this->mNumSubChannels; i++) {
                    if (this->mSubChannel[i]) {
                        this->mSubChannel[i]->setPaused(true);
                    }
                }

                this->mFinished = true;
                break;
            }
        }

        crit.leave();
    }

    delta = pcm - this->mLastPCM;
    if (delta < 0) {
        delta += sample->mLoopLength;
    }

    if (delta < 0) {
        delta = 0;
    }

    this->mSamplesPlayed += delta;
    this->mLastPCM = pcm;

    return FMOD_OK;
}

FMOD_RESULT ChannelStream::stop(bool ignoreenddelay, bool resetstate, bool force) {
    FMOD_RESULT result = FMOD_OK;
    FMOD_UINT_NATIVE id;

    this->mFinished = true;

    FMOD_OS_Thread_GetCurrentID(&id);

    if (!(this->mMode & FMOD_NONBLOCKING && id == this->mSystem->mMainThreadID)) {
        FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamUpdateCrit);
    }

    FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamRealchanCrit);

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        if (this->mSubChannel[i]) {
            if (resetstate) {
                this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ALLOCATED;
                this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_PLAYING;
                this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_PAUSED;
                this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_PAUSEDFORSETPOS;
                this->mSubChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ENDDELAY;
                this->mSubChannel[i]->mFlags |= CHANNELREAL_FLAG_STOPPED;
            }

            if (this->mSubChannel[i]->mDelayEnd && !ignoreenddelay) {
                this->mFlags |= CHANNELREAL_FLAG_ENDDELAY;
                this->mSubChannel[i]->mFlags |= CHANNELREAL_FLAG_ENDDELAY;
            }

            result = this->mSubChannel[i]->stop(ignoreenddelay, true, force);

            this->mSubChannel[i]->mSound = nullptr;
            this->mSubChannel[i]->mDSP = nullptr;
            this->mSubChannel[i]->mParent = nullptr;
            this->mSubChannel[i] = nullptr;
        }
    }

    FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamRealchanCrit);

    if (!(this->mMode & FMOD_NONBLOCKING) || id != this->mSystem->mMainThreadID) {
        FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamUpdateCrit);
    }

    FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamListCrit);

    if (this->mSystem->mStreamListChannelNext == &this->mStreamNode) {
        this->mSystem->mStreamListChannelNext = this->mStreamNode.getNext();
    }

    this->mStreamNode.removeNode();
    this->mStreamNode.setData(nullptr);

    FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamListCrit);

    return result;
}

FMOD_RESULT ChannelStream::stop(bool ignoreenddelay, bool resetstate) {
    return this->stop(ignoreenddelay, resetstate);
}

FMOD_RESULT ChannelStream::setPaused(bool paused) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->setPaused(paused);
    }

    return result;
}

FMOD_RESULT ChannelStream::setVolume(float volume) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mNumSubChannels > 1 && this->mParent->mLevels) {
        for (int32_t i = 0; i < this->mNumSubChannels; i++) {
            result = this->mSubChannel[i]->updateSpeakerLevels(volume);
        }
    } else {
        for (int32_t i = 0; i < this->mNumSubChannels; i++) {
            result = this->mSubChannel[i]->setVolume(volume);
        }
    }

    return result;
}

FMOD_RESULT ChannelStream::setFrequency(float frequency) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->setFrequency(frequency);
    }

    return result;
}

FMOD_RESULT ChannelStream::setPan(float pan, float fbpan) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        if (this->mNumSubChannels > 1) {
            if (this->mNumSubChannels == 2 || (this->mSound && this->mSound->mDefaultChannelMask == SPEAKER_ALLSTEREO)) {
                if (!(i & 1)) {
                    pan = -1.0f;
                } else {
                    pan = 1.0f;
                }
            }
        }

        result = this->mSubChannel[i]->setPan(pan, fbpan);
    }

    return result;
}

FMOD_RESULT ChannelStream::setDelay(uint32_t startdelay, uint32_t enddelay) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->setDelay(startdelay, enddelay);
    }

    return result;
}

FMOD_RESULT ChannelStream::setSpeakerMix(
    float frontleft,
    float frontright,
    float center,
    float lfe,
    float backleft,
    float backright,
    float sideleft,
    float sideright) {

    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->setSpeakerMix(
            frontleft,
            frontright,
            center,
            lfe,
            backleft,
            backright,
            sideleft,
            sideright);
    }

    return result;
}

FMOD_RESULT ChannelStream::setSpeakerLevels(int32_t speaker, float* levels, int32_t numlevels) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->setSpeakerLevels(
            speaker,
            levels,
            numlevels);
    }

    return result;
}

FMOD_RESULT ChannelStream::setPosition(uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result = FMOD_OK;
    FMOD_UINT_NATIVE id;
    bool paused;

    if (this->mFlags & CHANNELREAL_FLAG_STOPPED) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    auto stream = static_cast<Stream*>(this->mSound);
    if (!stream) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (postype == FMOD_TIMEUNIT_MS || postype == FMOD_TIMEUNIT_PCM || postype == FMOD_TIMEUNIT_PCMBYTES) {
        switch (postype) {
        case FMOD_TIMEUNIT_MS: {
            position = static_cast<uint32_t>(static_cast<float>(position) / 1000.0f * stream->mDefaultFrequency);
            postype = FMOD_TIMEUNIT_PCM;
            break;
        }
        case FMOD_TIMEUNIT_PCM: {
            break;
        }
        case FMOD_TIMEUNIT_PCMBYTES: {
            stream->getSamplesFromBytes(position, &position);
            postype = FMOD_TIMEUNIT_PCM;
            break;
        }
        default: {
            break;
        }
        }
    }

    if (postype == FMOD_TIMEUNIT_SENTENCE) {
        stream->mSubSoundIndex = stream->mSubSoundList[position];
        postype = FMOD_TIMEUNIT_MS;
        position = 0;
    } else if (postype == FMOD_TIMEUNIT_PCM) {
        if (stream->mSubSoundIndex == stream->mCodec->mSubSoundIndex && !(stream->mFlags & FMOD_SOUND_FLAG_WANTSTOFLUSH) && !(this->mFlags & CHANNELREAL_FLAG_PLAYING)) {
            return FMOD_OK;
        }
    }

    FMOD_OS_Thread_GetCurrentID(&id);
    this->mSubChannel[0]->getPaused(&paused);

    FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamUpdateCrit);

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        this->mSubChannel[i]->setPaused(true);
    }

    result = stream->setPosition(position, postype);
    if (result == FMOD_OK) {
        for (int32_t i = 0; i < this->mNumSubChannels; i++) {
            this->mSubChannel[i]->setPosition(0, FMOD_TIMEUNIT_PCM);
        }

        this->mLastPCM = 0;
        this->mDecodeOffset = 0;
        this->mPosition = position;
        this->mSamplesWritten = 0;
        this->mSamplesPlayed = 0;

        result = stream->flush();
    }

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        this->mSubChannel[i]->setPaused(paused);
    }

    FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamUpdateCrit);
    return result;
}

FMOD_RESULT ChannelStream::getPosition(uint32_t* position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;
    bool getsubsoundtime = false;

    if (!position) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto stream = static_cast<Stream*>(this->mSound);
    if (!stream) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (postype == FMOD_TIMEUNIT_SENTENCE_MS) {
        postype = FMOD_TIMEUNIT_MS;
        getsubsoundtime = true;
    } else if (postype == FMOD_TIMEUNIT_SENTENCE_PCM) {
        postype = FMOD_TIMEUNIT_PCM;
        getsubsoundtime = true;
    } else if (postype == FMOD_TIMEUNIT_SENTENCE_PCMBYTES) {
        postype = FMOD_TIMEUNIT_PCMBYTES;
        getsubsoundtime = true;
    } else if (postype == FMOD_TIMEUNIT_SENTENCE || postype == FMOD_TIMEUNIT_SENTENCE_SUBSOUND) {
        getsubsoundtime = true;
    }

    if (getsubsoundtime && !this->mSound->mSubSoundList) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (postype == FMOD_TIMEUNIT_MS ||
        postype == FMOD_TIMEUNIT_PCM ||
        postype == FMOD_TIMEUNIT_PCMBYTES ||
        postype == FMOD_TIMEUNIT_SENTENCE ||
        postype == FMOD_TIMEUNIT_SENTENCE_SUBSOUND) {

        uint32_t pos;
        int32_t currentsentenceid = 0;

        result = this->mSubChannel[0]->getPosition(&pos, FMOD_TIMEUNIT_PCM);
        if (result != FMOD_OK) {
            pos = 0;
        }

        // XXX: invented variable name
        int32_t samplesahead = static_cast<int32_t>(pos) - static_cast<int32_t>(this->mSamplesWritten % stream->mSample->mLength);
        if (samplesahead < 0) {
            samplesahead += stream->mSample->mLength;
        }

        pos = this->mPosition;

        while (samplesahead) {
            uint32_t endpoint;

            if (this->mMode & FMOD_LOOP_NORMAL && this->mLoopCount) {
                endpoint = this->mLoopStart + this->mLoopLength - 1;
            } else {
                endpoint = stream->mLength - 1;
            }

            // XXX: invented variable name
            auto currentsamplelength = (endpoint - pos) + 1;
            if (samplesahead < currentsamplelength) {
                currentsamplelength = samplesahead;
            }

            pos += currentsamplelength;

            if (endpoint < pos) {
                if (stream->mMode & FMOD_LOOP_NORMAL && this->mLoopCount) {
                    pos = this->mLoopStart;
                } else {
                    pos = stream->mLength;
                }
            }

            if (!currentsamplelength) {
                break;
            }

            samplesahead -= currentsamplelength;
        }

        if (getsubsoundtime) {
            for (int32_t i = 0; i < this->mSound->mSubSoundListNum; i++) {
                uint32_t lengthpcm = 0;

                auto sound = this->mSound->mSubSound[this->mSound->mSubSoundList[i]];

                if (sound) {
                    if (this->mSound->mSubSoundShared) {
                        FMOD_CODEC_WAVEFORMAT waveformat;

                        result = stream->mCodec->mDescription.getwaveformat(stream->mCodec, stream->mSubSoundList[i], &waveformat);
                        if (result != FMOD_OK) {
                            return result;
                        }

                        lengthpcm = waveformat.lengthpcm;
                    } else {
                        lengthpcm = sound->mLength;
                    }
                }

                if (pos >= lengthpcm) {
                    pos -= lengthpcm;
                } else {
                    break;
                }

                currentsentenceid++;
            }
        }

        if (postype == FMOD_TIMEUNIT_SENTENCE) {
            *position = currentsentenceid;
        } else if (postype == FMOD_TIMEUNIT_SENTENCE_SUBSOUND) {
            *position = this->mSound->mSubSoundList[currentsentenceid];
        } else if (postype == FMOD_TIMEUNIT_PCM) {
            *position = pos;
        } else if (postype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getBytesFromSamples(pos, position, this->mSound->mChannels, this->mSound->mFormat);
        } else if (postype == FMOD_TIMEUNIT_MS) {
            *position = static_cast<uint32_t>(static_cast<float>(pos) / this->mSound->mDefaultFrequency * 1000.0f);
        }

        return FMOD_OK;
    }

    return stream->getPosition(position, postype);
}

FMOD_RESULT ChannelStream::setLoopPoints(uint32_t loopstart, uint32_t looplength) {
    FMOD_RESULT result;

    result = ChannelReal::setLoopPoints(loopstart, looplength);
    if (result != FMOD_OK) {
        return FMOD_OK;
    }

    result = this->mSound->setLoopPoints(
        loopstart,
        FMOD_TIMEUNIT_PCM,
        loopstart + looplength - 1,
        FMOD_TIMEUNIT_PCM);
    if (result != FMOD_OK) {
        return FMOD_OK;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelStream::setLoopCount(int32_t loopcount) {
    FMOD_RESULT result;

    result = ChannelReal::setLoopCount(loopcount);
    if (result == FMOD_OK) {
        this->mSound->setLoopCount(loopcount);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelStream::set3DAttributes() {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->set3DAttributes();
    }

    return result;
}

FMOD_RESULT ChannelStream::set3DMinMaxDistance() {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->set3DMinMaxDistance();
    }

    return result;
}

FMOD_RESULT ChannelStream::set3DOcclusion(float directOcclusion, float reverbOcclusion) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->set3DOcclusion(
            directOcclusion,
            reverbOcclusion);
    }

    return result;
}

FMOD_RESULT ChannelStream::setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->setReverbProperties(prop);
    }

    return result;
}

FMOD_RESULT ChannelStream::getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop) {
    FMOD_RESULT result = FMOD_OK;

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->getReverbProperties(prop);
    }

    return result;
}

FMOD_RESULT ChannelStream::isPlaying(bool* isplaying, bool includethreadlatency) {
    *isplaying = !this->mFinished;
    return FMOD_OK;
}

FMOD_RESULT ChannelStream::getSpectrum(
    float* spectrumarray,
    int32_t numentries,
    int32_t channeloffset,
    FMOD_DSP_FFT_WINDOW windowtype) {

    return this->mSubChannel[0]->getSpectrum(
        spectrumarray,
        numentries,
        channeloffset,
        windowtype);
}

FMOD_RESULT ChannelStream::getWaveData(float* wavearray, int32_t numvalues, int32_t channeloffset) {
    return this->mSubChannel[0]->getWaveData(
        wavearray,
        numvalues,
        channeloffset);
}

FMOD_RESULT ChannelStream::getDSPHead(DSPI** dsp) {
    return this->mSubChannel[0]->getDSPHead(
        dsp);
}

FMOD_RESULT ChannelStream::setMode(FMOD_MODE mode) {
    FMOD_RESULT result;

    result = ChannelReal::setMode(mode);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mSound->setMode(mode);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubChannels; i++) {
        result = this->mSubChannel[i]->setMode(mode & ~(FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI));
    }

    return result;
}

} // namespace FMOD
