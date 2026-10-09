#include "xmodex/channel/ChannelSoftware.hpp"
#include "fmod.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/ReverbI.hpp"
#include "xmodex/SpeakerMap.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelReal.hpp"
#include "xmodex/codec/Codec.hpp"
#include "xmodex/codec/CodecMPEG.hpp"
#include "xmodex/codec/CodecRaw.hpp"
#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/dsp/DSPCodec.hpp"
#include "xmodex/dsp/DSPConnection.hpp"
#include "xmodex/dsp/DSPFFT.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/dsp/DSPResampler.hpp"
#include "xmodex/dsp/DSPWaveTable.hpp"
#include "xmodex/os/CriticalSection.hpp"
#include "xmodex/sound/SampleSoftware.hpp"
#include "xmodex/sound/SoundGroupI.hpp"

namespace FMOD {

ChannelSoftware::ChannelSoftware() {
    this->mDSPWaveTable = nullptr;
    this->mDSPHead = nullptr;
    this->mDSPResampler = nullptr;
    this->mDSPCodec = nullptr;
    this->mDSPCodecStopped = nullptr;
    this->mDSPLowPass = nullptr;
    this->mDSP = nullptr;
    this->mDSPConnection = nullptr;
    this->mDSPReverb = nullptr;
}

FMOD_RESULT ChannelSoftware::addToReverbs(DSPI* dsptarget) {
    FMOD_RESULT result;
    FMOD_REVERB_CHANNELPROPERTIES channelprops;
    DSPConnection* dspconnection;

    channelprops.ObstructionLFRatio = 0.0f;
    channelprops.OcclusionLFRatio = 0.25f;
    channelprops.OcclusionRoomRatio = 1.5f;
    channelprops.OcclusionDirectRatio = 1.0f;
    channelprops.Direct = 0;
    channelprops.ExclusionLFRatio = 1.0f;
    channelprops.DirectHF = 0;
    channelprops.Room = 0;
    channelprops.DopplerFactor = 0.0f;
    channelprops.RoomHF = 0;
    channelprops.RolloffFactor = 0.0f;
    channelprops.Obstruction = 0;
    channelprops.RoomRolloffFactor = 0.0f;
    channelprops.Occlusion = 0;
    channelprops.Exclusion = 0;
    channelprops.OutsideVolumeHF = 0;
    channelprops.Flags = 0;
    channelprops.AirAbsorptionFactor = 1.0f;

    if (!dsptarget) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mSystem->mReverbGlobal.mDSP) {
        result = this->mSystem->mReverbGlobal.mDSP->addInputQueued(
            dsptarget,
            false,
            nullptr,
            &dspconnection);
        if (result != FMOD_OK) {
            return result;
        }

        this->mSystem->mReverbGlobal.setChanProperties(this->mParent->mIndex, &channelprops, dspconnection);
        this->mSystem->mReverbGlobal.setPresenceGain(this->mParent->mIndex, 1.0f);
    }

    if (this->mSystem->mReverb3D.mDSP) {
        result = this->mSystem->mReverb3D.mDSP->addInputQueued(
            dsptarget,
            false,
            nullptr,
            &dspconnection);
        if (result != FMOD_OK) {
            return result;
        }

        this->mSystem->mReverb3D.setChanProperties(this->mParent->mIndex, &channelprops, dspconnection);
        this->mSystem->mReverb3D.setPresenceGain(this->mParent->mIndex, 1.0f);
    }

    auto reverb = static_cast<ReverbI*>(this->mSystem->mReverb3DHead.getNext());
    while (reverb != &this->mSystem->mReverb3DHead) {
        if (reverb->mDSP) {
            result = reverb->mDSP->addInputQueued(
                dsptarget,
                false,
                nullptr,
                &dspconnection);
            if (result != FMOD_OK) {
                return result;
            }

            reverb->setChanProperties(this->mParent->mIndex, &channelprops, dspconnection);
            reverb->setPresenceGain(this->mParent->mIndex, 1.0f);
        }

        reverb = static_cast<ReverbI*>(reverb->getNext());
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setupDSPCodec(DSPI* dsp) {
    FMOD_RESULT result;

    auto dspcodec = static_cast<DSPCodec*>(dsp);
    auto codec = dspcodec->mCodec;
    auto sample = static_cast<SampleSoftware*>(this->mSound);
    auto soundcodec = sample->mCodec ? sample->mCodec : sample->mSubSoundParent->mCodec;

    if (!soundcodec) {
        return FMOD_ERR_INTERNAL;
    }

    codec->mPCMBufferLength = soundcodec->mPCMBufferLength;
    codec->mPCMBufferLengthBytes = soundcodec->mPCMBufferLength * sizeof(int16_t) * dsp->mDescription.channels;

    if (!codec->waveformat) {
        return FMOD_ERR_INTERNAL;
    }

    soundcodec->mDescription.getwaveformat(soundcodec, sample->mSubSoundIndex, codec->waveformat);

    codec->mFlags = soundcodec->mFlags;
    dspcodec->mNewPosition = 0xFFFFFFFF;
    dspcodec->mCodec->mFile = &dspcodec->mMemoryFile;

    dspcodec->mMemoryFile.init(sample->mLengthBytes, 0);
    dspcodec->mMemoryFile.mMem = sample->mBuffer;

    if (sample->mType == FMOD_SOUND_TYPE_WAV && sample->mFormat == FMOD_SOUND_FORMAT_IMAADPCM) {

        auto srcwav = static_cast<CodecWav*>(soundcodec);
        auto destwav = static_cast<CodecWav*>(soundcodec);
        destwav->mSamplesPerADPCMBlock = srcwav->mSamplesPerADPCMBlock;
        destwav->mReadBufferLength = srcwav->mReadBufferLength;

    } else if (sample->mType == FMOD_SOUND_TYPE_RAW && sample->mFormat == FMOD_SOUND_FORMAT_IMAADPCM) {
        auto srcwav = static_cast<CodecRaw*>(soundcodec);
        auto destwav = static_cast<CodecWav*>(codec);

        destwav->mSamplesPerADPCMBlock = srcwav->mSamplesPerADPCMBlock;
        destwav->mReadBufferLength = srcwav->mReadBufferLength;
    } else if (sample->mType == FMOD_SOUND_TYPE_FSB) {
        // TODO
        return FMOD_ERR_INTERNAL;
    } else if (sample->mFormat == FMOD_SOUND_FORMAT_MPEG) {
        auto srcmpeg = static_cast<CodecMPEG*>(soundcodec);
        auto destmpeg = static_cast<CodecMPEG*>(codec);

        destmpeg->mPCMFrameLengthBytes = srcmpeg->mPCMFrameLengthBytes;
    }

    dsp->mDescription.channels = sample->mChannels;

    result = dsp->setTargetFrequency(static_cast<int32_t>(this->mParent->mChannelGroup->mDSPHead->mDefaultFrequency));
    if (result != FMOD_OK) {
        return result;
    }

    this->mDSPCodec = dspcodec;
    this->mMinFrequency = 0.0f;

    dspcodec->mLength = this->mSound->mLength;
    this->mDSPCodec->mLoopStart = this->mLoopStart;
    this->mDSPCodec->mLoopLength = this->mLoopLength;
    this->mDSPCodec->mLoopCount = this->mLoopCount;
    this->mDSPCodec->mMode = this->mMode;
    this->mDSPCodec->mDefaultFrequency = this->mSound->mDefaultFrequency;

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::updateDirectMix(float volume) {
    FMOD_RESULT result;
    float gainlin;

    gainlin = volume;
    gainlin *= this->mParent->mFadeVolume;
    gainlin *= this->mParent->mReverbDryVolume;
    gainlin *= ((1.0f - this->mParent->m3DPanLevel) + (this->mParent->mVolume3D * this->mParent->m3DPanLevel));
    gainlin *= this->mParent->mChannelGroup->mRealVolume;
    gainlin *= ((1.0f - this->mParent->m3DPanLevel) + (this->mParent->mConeVolume3D * this->mParent->m3DPanLevel));
    if (this->mSound && this->mSound->mSoundGroup) {
        gainlin *= this->mSound->mSoundGroup->mVolume;
    }

    float transmission = this->mParent->mChannelGroup->mRealDirectOcclusionVolume * this->mParent->mDirectOcclusionVolume;

    if (this->mDSPLowPass) {
        float hrtf_cutoff = 22050.0f;

        if (this->mSystem->mFlags & FMOD_INIT_SOFTWARE_HRTF) {
            float hrtf_min_angle = this->mSystem->mAdvancedSettings.HRTFMinAngle / 2.0f;
            float hrtf_max_angle = this->mSystem->mAdvancedSettings.HRTFMaxAngle / 2.0f;

            float abs_angle = (this->mAngleToListener <= 180.0f) ? this->mAngleToListener : 360.0f - this->mAngleToListener;

            if (abs_angle <= hrtf_min_angle) {
                hrtf_cutoff = 22050.0f;
            } else if (abs_angle >= hrtf_max_angle) {
                hrtf_cutoff = this->mSystem->mAdvancedSettings.HRTFFreq;
            } else {
                float filter_strength = (abs_angle - hrtf_min_angle) / (hrtf_max_angle - hrtf_min_angle);
                float range = 22050.0f - this->mSystem->mAdvancedSettings.HRTFFreq;

                hrtf_cutoff = this->mSystem->mAdvancedSettings.HRTFFreq + range * (1.0f - filter_strength);
            }
        }

        hrtf_cutoff = (((1.0f - this->mParent->m3DPanLevel) * 22050.0f) + (hrtf_cutoff * this->mParent->m3DPanLevel));

        if (transmission < 1.0f || hrtf_cutoff < 22050.0f) {
            float cutoff;

            this->mDSPLowPass->setBypass(false);

            cutoff = 22050.0f * transmission * transmission;

            if (hrtf_cutoff < cutoff) {
                cutoff = hrtf_cutoff;
            }

            this->mDSPLowPass->setParameter(FMOD_DSP_LOWPASS_SIMPLE_CUTOFF, cutoff);
        } else {
            this->mDSPLowPass->setBypass(true);
        }
    }

    result = this->mDSPConnection->setMix(gainlin);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::updateReverbMix(ReverbI* reverb, float volume) {
    FMOD_RESULT result;
    FMOD_REVERB_CHANNELPROPERTIES props;
    DSPConnection* connection;
    bool morphing;
    float gainlin;

    if (!reverb) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!reverb->mDSP) {
        return FMOD_OK;
    }

    result = reverb->getChanProperties(this->mParent->mIndex, &props, &connection);
    if (result != FMOD_OK) {
        return result;
    }

    if (!connection) {
        return FMOD_OK;
    }

    result = this->mSystem->get3DReverbActive(&morphing);
    if (result != FMOD_OK) {
        return result;
    }

    float transmission = (1.0f - this->mParent->mReverbOcclusion) *
                         this->mParent->mChannelGroup->mRealReverbOcclusionVolume;

    if (reverb == &this->mSystem->mReverbGlobal) {
        if (this->mMode & FMOD_3D) {
            if (morphing) {
                gainlin = 0.0f;
            } else {
                gainlin = FMOD_POW(10.0f, props.Room / 2000.0f);

                gainlin *= transmission;
                gainlin *= this->mParent->mFadeVolume;
                gainlin *= this->mParent->mVolume3D;
                gainlin *= this->mParent->mChannelGroup->mRealVolume;
                if (this->mSound && this->mSound->mSoundGroup) {
                    gainlin *= this->mSound->mSoundGroup->mVolume;
                }
                gainlin *= volume;
            }
        } else {
            gainlin = FMOD_POW(10.0f, props.Room / 2000.0f);

            gainlin *= this->mParent->mChannelGroup->mRealVolume;
            gainlin *= volume;
        }
    }

    else if (reverb == &this->mSystem->mReverb3D) {
        if (this->mMode & FMOD_3D) {
            gainlin = FMOD_POW(10.0f, props.Room / 2000.0f);

            gainlin *= transmission;
            gainlin *= this->mParent->mFadeVolume;
            gainlin *= this->mParent->mVolume3D;
            gainlin *= this->mParent->mChannelGroup->mRealVolume;
            if (this->mSound && this->mSound->mSoundGroup) {
                gainlin *= this->mSound->mSoundGroup->mVolume;
            }

            gainlin *= volume;
        } else {
            gainlin = 0.0f;
        }
    } else {
        if (this->mMode & FMOD_2D) {
            gainlin = 0.0f;
        } else {
            float source_gain;
            reverb->getPresenceGain(this->mParent->mIndex, &source_gain);

            float listener_gain = reverb->getGain();
            gainlin = (listener_gain > source_gain) ? listener_gain : source_gain;

            gainlin *= FMOD_POW(10.0f, props.Room / 2000.0f);

            gainlin *= transmission;
            gainlin *= this->mParent->mFadeVolume;
            gainlin *= this->mParent->mVolume3D;
            gainlin *= this->mParent->mChannelGroup->mRealVolume;
            if (this->mSound && this->mSound->mSoundGroup) {
                gainlin *= this->mSound->mSoundGroup->mVolume;
            }

            gainlin *= volume;
        }
    }

    result = connection->setMix(gainlin);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::init(
    int32_t index,
    SystemI* system,
    Output* output,
    DSPI* dspmixtarget) {

    FMOD_RESULT result;
    FMOD_DSP_DESCRIPTION_EX descriptionex;

    ChannelReal::init(index, system, output, dspmixtarget);

    FMOD_memset(&descriptionex, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));
    FMOD_strcpy(descriptionex.name, "FMOD Channel DSPHead Unit");
    descriptionex.version = 0x10100;
    descriptionex.mCategory = FMOD_DSP_CATEGORY_FILTER;
    descriptionex.mFormat = FMOD_SOUND_FORMAT_PCMFLOAT;

    result = this->mSystem->createDSP(&descriptionex, &this->mDSPHead, false);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mSystem->mFlags & (FMOD_INIT_SOFTWARE_OCCLUSION | FMOD_INIT_SOFTWARE_HRTF)) {
        result = this->mSystem->createDSPByType(FMOD_DSP_TYPE_LOWPASS_SIMPLE, &this->mDSPLowPass);
        if (result != FMOD_OK) {
            return result;
        }
    }

    FMOD_memset(&descriptionex, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));
    FMOD_strcpy(descriptionex.name, "FMOD WaveTable Unit");
    descriptionex.channels = dspmixtarget->mDescription.channels;
    descriptionex.mFormat = dspmixtarget->mDescription.mFormat;
    descriptionex.read = nullptr;
    descriptionex.version = 0x10100;

    descriptionex.setparameter = DSPWaveTable::setParameterCallback;
    descriptionex.getparameter = DSPWaveTable::getParameterCallback;
    descriptionex.setposition = DSPWaveTable::setPositionCallback;

    descriptionex.mCategory = FMOD_DSP_CATEGORY_WAVETABLE;

    this->mDSPWaveTable = &this->mDSPWaveTableMemory;

    result = this->mSystem->createDSP(&descriptionex, reinterpret_cast<DSPI**>(&this->mDSPWaveTable), false);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mDSPWaveTable->setUserData(this);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mDSPWaveTable->setTargetFrequency(static_cast<int32_t>(dspmixtarget->mDefaultFrequency));
    if (result != FMOD_OK) {
        return result;
    }

    this->mMinFrequency = -this->mMaxFrequency;

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::moveChannelGroup(ChannelGroupI* oldchannelgroup, ChannelGroupI* newchannelgroup) {
    FMOD_RESULT result;

    if (oldchannelgroup == newchannelgroup) {
        return FMOD_OK;
    }

    if (this->mDSPLowPass) {
        if (oldchannelgroup && oldchannelgroup->mDSPMixTarget) {
            result = oldchannelgroup->mDSPMixTarget->disconnectFrom(this->mDSPHead);
            if (result != FMOD_OK) {
                return result;
            }
        }

        result = newchannelgroup->mDSPMixTarget->addInputQueued(
            this->mDSPHead,
            false,
            this->mDSPConnection,
            &this->mDSPConnection);
        if (result != FMOD_OK) {
            return result;
        }
    } else {
        if (oldchannelgroup && oldchannelgroup->mDSPMixTarget) {
            result = oldchannelgroup->mDSPMixTarget->disconnectFrom(
                this->mDSPHead);
            if (result != FMOD_OK) {
                return result;
            }

            result = newchannelgroup->mDSPMixTarget->addInputQueued(
                this->mDSPHead,
                false,
                this->mDSPConnection,
                &this->mDSPConnection);
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::alloc(DSPI* dsp) {
    FMOD_RESULT result;

    result = ChannelReal::alloc();
    if (result != FMOD_OK) {
        return result;
    }

    this->mDSPCodec = nullptr;

    {
        FMOD_DSP_DESCRIPTION_EX descriptionex;
        FMOD_memset(&descriptionex, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));
        FMOD_strcpy(descriptionex.name, "FMOD Resampler Unit");
        descriptionex.version = 0x10100;
        descriptionex.channels = 0;
        descriptionex.mCategory = FMOD_DSP_CATEGORY_RESAMPLER;

        result = this->mSystem->createDSP(&descriptionex, reinterpret_cast<DSPI**>(&this->mDSPResampler), true);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mDSPResampler->setUserData(this);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mDSPResampler->setTargetFrequency(static_cast<int32_t>(this->mParent->mChannelGroup->mDSPHead->mDefaultFrequency));
        if (result != FMOD_OK) {
            return result;
        }

        this->mMinFrequency = 0.0f;
    }

    result = this->mDSPHead->disconnectFrom(nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDSPLowPass) {
        result = this->mDSPLowPass->disconnectFrom(nullptr);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->mDSPWaveTable->disconnectFrom(nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mDSPHead->addInputQueued(
        this->mDSPResampler,
        false,
        nullptr,
        nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mDSPResampler->addInputQueued(
        dsp,
        false,
        nullptr,
        nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mParent->mChannelGroup->mDSPHead->addInputQueued(
        this->mDSPHead,
        false,
        nullptr,
        &this->mDSPConnection);
    if (result != FMOD_OK) {
        return result;
    }

    this->mDSPReverb = this->mDSPResampler;

    result = this->addToReverbs(this->mDSPReverb);
    if (result != FMOD_OK) {
        return result;
    }

    auto resampler = this->mDSPResampler;
    resampler->mLength = this->mLength;
    resampler->mLoopStart = this->mLoopStart;
    resampler->mLoopLength = this->mLoopLength;
    resampler->mLoopCount = this->mLoopCount;
    resampler->mMode = this->mMode;

    if (!this->mDSPWaveTable) {
        return FMOD_ERR_INTERNAL;
    }

    this->mDSPWaveTable->mSound = nullptr;

    this->mDSPHead->setFinished(false, false);
    this->mDSPHead->setActive(false);

    this->mDSPResampler->setFinished(false, false);
    this->mDSPResampler->setActive(false);

    dsp->setFinished(false, false);
    dsp->setActive(false);

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::alloc() {
    FMOD_RESULT result;

    result = ChannelReal::alloc();
    if (result != FMOD_OK) {
        return result;
    }

    if (!(this->mMode & FMOD_CREATECOMPRESSEDSAMPLE)) {
        auto dspwave = this->mDSPWaveTable;
        if (!dspwave) {
            return FMOD_ERR_INTERNAL;
        }

        this->mDSPCodec = nullptr;
        this->mDSPCodecStopped = nullptr;

        result = this->mDSPHead->disconnectFrom(nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDSPLowPass) {
            result = this->mDSPLowPass->disconnectFrom(nullptr);
            if (result != FMOD_OK) {
                return result;
            }
        }

        result = this->mDSPWaveTable->disconnectFrom(nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDSPLowPass) {
            result = this->mParent->mChannelGroup->mDSPHead->addInputQueued(
                this->mDSPHead,
                false,
                nullptr,
                &this->mDSPConnection);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->mDSPHead->addInputQueued(
                this->mDSPLowPass,
                false,
                nullptr,
                nullptr);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->mDSPLowPass->addInputQueued(
                this->mDSPWaveTable,
                false,
                nullptr,
                nullptr);
            if (result != FMOD_OK) {
                return result;
            }
        } else {
            result = this->mParent->mChannelGroup->mDSPHead->addInputQueued(
                this->mDSPHead,
                false,
                nullptr,
                &this->mDSPConnection);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->mDSPHead->addInputQueued(
                this->mDSPWaveTable,
                false,
                nullptr,
                nullptr);
            if (result != FMOD_OK) {
                return result;
            }
        }

        if (!(this->mFlags & CHANNELREAL_FLAG_NOREVERB)) {
            this->mDSPReverb = this->mDSPWaveTable;

            result = this->addToReverbs(this->mDSPReverb);
            if (result != FMOD_OK) {
                return result;
            }
        }

        result = this->setLoopPoints(
            this->mSound->mLoopStart,
            this->mSound->mLoopLength);
        if (result != FMOD_OK) {
            return result;
        }

        this->mMinFrequency = -this->mMaxFrequency;

        dspwave->mChannel = this;
        dspwave->mSound = this->mSound;
        dspwave->mPosition.mHi = 0;
        dspwave->mPosition.mLo = 0;
        dspwave->mDirection = DSPWAVETABLE_SPEEDDIR_FORWARDS;

        this->mDSPHead->setFinished(false, false);
        this->mDSPHead->setActive(false);

        if (this->mDSPLowPass) {
            this->mDSPLowPass->setFinished(false, false);
            this->mDSPLowPass->setActive(false);
        }

        this->mDSPWaveTable->setFinished(false, false);
        this->mDSPWaveTable->setActive(false);

        return FMOD_OK;
    } else {
        DSPCodec* dsp;
        bool nodsplock = true;

        if (this->mDSPCodecStopped) {
            if (!*this->mDSPCodecStopped->mWantsToFinish && (this->mDSPCodecStopped->mFlags & FMOD_DSP_FLAG_FINISHED)) {
                nodsplock = true;
                this->mDSPCodecStopped = nullptr;
            } else {
                nodsplock = false;
                this->mSystem->flushDSPConnectionRequests(true);
            }
        }

        result = this->mDSPHead->disconnectFrom(nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDSPLowPass) {
            result = this->mDSPLowPass->disconnectFrom(nullptr);
            if (result != FMOD_OK) {
                return result;
            }
        }

        if (this->mDSPWaveTable) {
            result = this->mDSPWaveTable->disconnectFrom(nullptr);
            if (result != FMOD_OK) {
                return result;
            }
        }

        result = this->mSystem->allocDSPCodec(
            this->mSound->mFormat,
            reinterpret_cast<DSPI**>(&dsp));

        if (result != FMOD_OK) {
            return result;
        }

        if (!nodsplock) {
            FMOD_OS_CriticalSection_Enter(this->mSystem->mDSPCrit);
        }

        result = this->setupDSPCodec(dsp);

        if (!nodsplock) {
            FMOD_OS_CriticalSection_Leave(this->mSystem->mDSPCrit);
        }

        if (result != FMOD_OK) {
            return result;
        }

        if (this->mDSPLowPass) {
            result = this->mParent->mChannelGroup->mDSPHead->addInputQueued(
                this->mDSPHead,
                false,
                nullptr,
                &this->mDSPConnection);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->mDSPHead->addInputQueued(
                this->mDSPLowPass,
                false,
                nullptr,
                nullptr);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->mDSPLowPass->addInputQueued(
                dsp,
                false,
                nullptr,
                nullptr);
        } else {
            result = this->mParent->mChannelGroup->mDSPHead->addInputQueued(
                this->mDSPHead,
                false,
                nullptr,
                &this->mDSPConnection);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->mDSPHead->addInputQueued(
                dsp,
                false,
                nullptr,
                nullptr);
            if (result != FMOD_OK) {
                return result;
            }
        }

        if (!(this->mFlags & CHANNELREAL_FLAG_NOREVERB)) {
            this->mDSPReverb = dsp;

            result = this->addToReverbs(this->mDSPReverb);
            if (result != FMOD_OK) {
                return result;
            }
        }

        this->mDSPHead->setFinished(false, false);
        this->mDSPHead->setActive(false);

        dsp->setFinished(false, false);
        dsp->setActive(true);

        return FMOD_OK;
    }
}

FMOD_RESULT ChannelSoftware::start() {
    if (!(this->mFlags & CHANNELREAL_FLAG_PAUSED)) {
        this->mDSPHead->setActive(true);
    }

    if (this->mSound) {
        this->mDSPWaveTable->setActive(true);
    }

    if (this->mDSPResampler) {
        this->mDSPResampler->setActive(true);
    }

    if (this->mDSPLowPass) {
        this->mDSPLowPass->setActive(true);
    }

    if (this->mDSPCodec) {
        this->mDSPCodec->setActive(true);
    }

    if (this->mDSP) {
        this->mDSP->setActive(true);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::stop(bool ignoreenddelay, bool resetstate, bool force) {
    FMOD_RESULT result;

    if (!(this->mFlags & CHANNELREAL_FLAG_ENDDELAY) && !ignoreenddelay) {
        return FMOD_OK;
    }

    if (this->mDSPHead) {
        this->mDSPHead->disconnectAll(false, true);
        this->mDSPHead->setFinished(true, force);
        this->mDSPHead->setActive(false);
    }

    if (this->mDSPCodec) {
        this->mDSPCodec->disconnectAll(false, true);
        this->mDSPCodec->setFinished(true, force);
        this->mDSPCodec->setActive(false);
        this->mDSPCodec->mPool->mAllocated[this->mDSPCodec->mPoolIndex] = false;
        this->mDSPCodecStopped = this->mDSPCodec;
        this->mDSPCodec = nullptr;
    }

    if (this->mDSPResampler) {
        this->mDSPResampler->setFinished(true, force);
        this->mDSPResampler->setActive(false);
        this->mDSPResampler->release(true);
        this->mDSPResampler = nullptr;
    }

    if (this->mDSPWaveTable) {
        this->mDSPWaveTable->disconnectAll(false, true);
        this->mDSPWaveTable->setFinished(true, force);
        this->mDSPWaveTable->setActive(false);
        this->mDSPWaveTable->mFlags &= ~FMOD_DSP_FLAG_VISITED;
    }

    if (this->mDSP) {
        int32_t numoutputs;
        DSPI* prev;

        result = this->mDSP->getNumOutputs(&numoutputs);
        if (result != FMOD_OK) {
            return result;
        }

        for (int32_t i = 0; i < numoutputs; i++) {
            result = this->mDSP->getOutput(i, &prev);
            if (result == FMOD_OK) {
                result = prev->disconnectFrom(this->mDSP);
                if (result != FMOD_OK) {
                    return result;
                }
            }
        }
    }

    this->mDSPReverb = nullptr;

    ChannelReal::stop(ignoreenddelay, true);

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::stop(bool ignoreenddelay, bool resetstate) {
    return this->stop(ignoreenddelay, resetstate, false);
}

FMOD_RESULT ChannelSoftware::setPaused(bool paused) {
    FMOD_RESULT result;

    result = this->mDSPHead->setActive(!paused);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDSPReverb) {
        this->mDSPReverb->setActive(!paused);
    }

    return ChannelReal::setPaused(paused);
}

FMOD_RESULT ChannelSoftware::setVolume(float volume) {
    FMOD_RESULT result;

    if (this->mSubChannelIndex > 0) {
        return FMOD_OK;
    }

    result = this->updateDirectMix(volume);
    if (result != FMOD_OK) {
        return result;
    }

    if (!(this->mFlags & CHANNELREAL_FLAG_NOREVERB)) {
        result = this->updateReverbMix(&this->mSystem->mReverbGlobal, volume);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->updateReverbMix(&this->mSystem->mReverb3D, volume);
        if (result != FMOD_OK) {
            return result;
        }

        auto reverb = static_cast<ReverbI*>(this->mSystem->mReverb3DHead.getNext());

        while (reverb != &this->mSystem->mReverb3DHead) {
            if (reverb->getMode() == FMOD_REVERB_PHYSICAL) {
                result = this->updateReverbMix(reverb, volume);
                if (result != FMOD_OK) {
                    return result;
                }
            }

            reverb = static_cast<ReverbI*>(reverb->getNext());
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setFrequency(float frequency) {
    if (this->mDSPResampler || this->mDSPCodec) {
        DSPResampler* dspresampler;

        if (this->mDSPCodec) {
            dspresampler = static_cast<DSPResampler*>(this->mDSPCodec);
        } else {
            dspresampler = static_cast<DSPResampler*>(this->mDSPResampler);
        }
        if (!dspresampler) {
            return FMOD_ERR_INVALID_PARAM;
        }

        frequency *= ((1.0f - this->mParent->m3DPanLevel) + (this->mParent->mPitch3D * this->mParent->m3DPanLevel));
        frequency *= this->mParent->mChannelGroup->mRealPitch;

        if (frequency > this->mMaxFrequency) {
            frequency = this->mMaxFrequency;
        }
        if (frequency < this->mMinFrequency) {
            frequency = this->mMinFrequency;
        }

        return dspresampler->setFrequency(frequency);
    } else if (this->mDSPWaveTable) {
        auto dspwave = static_cast<DSPWaveTable*>(this->mDSPWaveTable);
        if (!dspwave) {
            return FMOD_ERR_INTERNAL;
        }

        frequency *= ((1.0f - this->mParent->m3DPanLevel) + (this->mParent->mPitch3D * this->mParent->m3DPanLevel));
        frequency *= this->mParent->mChannelGroup->mRealPitch;

        if (frequency > this->mMaxFrequency) {
            frequency = this->mMaxFrequency;
        }
        if (frequency < this->mMinFrequency) {
            frequency = this->mMinFrequency;
        }

        return dspwave->setFrequency(frequency);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setPan(float lrpan, float fbpan) {
    float pan = (lrpan + 1.0f) / 2.0f;
    float l;
    float r;
    int32_t numchannels;
    uint32_t channelmask = 0;

    if (this->mSound) {
        numchannels = this->mSound->mChannels;
        channelmask = this->mSound->mDefaultChannelMask;
    } else if (this->mDSP) {
        numchannels = this->mDSP->mDescription.channels;
    } else {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (numchannels == 1) {
        l = FMOD_SQRT(1.0f - pan);
        r = FMOD_SQRT(pan);

        return this->setSpeakerMix(
            l,
            r,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f);
    } else {
        if (pan <= 0.5f) {
            l = 1.0f;
            r = pan * 2.0f;
        } else {
            l = (1.0f - pan) * 2.0f;
            r = 1.0f;
        }

        if (numchannels == 2 && !(channelmask & SPEAKER_ALLMONO)) {
            return this->setSpeakerMix(
                l,
                r,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f);
        } else {
            return this->setSpeakerMix(
                l,
                r,
                1.0f,
                1.0f,
                l,
                r,
                l,
                r);
        }
    }
}

FMOD_RESULT ChannelSoftware::setSpeakerMix(
    float frontleft,
    float frontright,
    float center,
    float lfe,
    float backleft,
    float backright,
    float sideleft,
    float sideright) {

    FMOD_RESULT result;
    int32_t numinputlevels;
    float levels[DSP_MAXLEVELS_OUT * DSP_MAXLEVELS_IN];
    int32_t numchannels;
    FMOD_SPEAKERMAPTYPE speakermap = FMOD_SPEAKERMAPTYPE_DEFAULT;

    if (this->mSubChannelIndex > 0) {
        return FMOD_OK;
    }

    if (this->mSound) {
        numchannels = this->mSound->mChannels;

        if (this->mSound->mDefaultChannelMask & SPEAKER_ALLMONO) {
            speakermap = FMOD_SPEAKERMAPTYPE_ALLMONO;
        } else if (this->mSound->mDefaultChannelMask & SPEAKER_ALLSTEREO) {
            speakermap = FMOD_SPEAKERMAPTYPE_ALLSTEREO;
        }
    } else if (this->mDSP) {
        numchannels = this->mDSP->mDescription.channels;
    } else {
        return FMOD_ERR_INVALID_HANDLE;
    }

    result = DSPI::calculateSpeakerLevels(
        frontleft,
        frontright,
        center,
        lfe,
        backleft,
        backright,
        sideleft,
        sideright,
        this->mSystem->mSpeakerMode,
        numchannels,
        speakermap,
        levels,
        &numinputlevels);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mParent->mFlags & CHANNELI_FLAG_USEDINPUTMIX) {
        for (int32_t i = 0; i < this->mSystem->mMaxOutputChannels; i++) {
            for (int32_t j = 0; j < numinputlevels; j++) {
                levels[(i * numinputlevels) + j] *= this->mParent->mInputMix[j];
            }
        }
    }

    result = this->mDSPConnection->setLevels(levels, numinputlevels);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setSpeakerLevels(int32_t speaker, float* levels, int32_t numlevels) {
    FMOD_RESULT result;
    float clevels[DSP_MAXLEVELS_OUT][DSP_MAXLEVELS_IN];

    if (this->mSubChannelIndex > 0) {
        return FMOD_OK;
    }

    result = this->mDSPConnection->getLevels(&clevels[0][0], DSP_MAXLEVELS_IN);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < numlevels; i++) {
        clevels[speaker][i] = levels[i] * this->mParent->mInputMix[i];
    }

    result = this->mDSPConnection->setLevels(&clevels[0][0], DSP_MAXLEVELS_IN);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setPosition(uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_SOUND_FORMAT format;
    float frequency;
    int32_t channels;
    uint32_t endpoint;
    uint32_t pcm = 0;

    if (this->mSubChannelIndex > 0) {
        return FMOD_OK;
    }

    if (postype != FMOD_TIMEUNIT_MS && postype != FMOD_TIMEUNIT_PCM && postype != FMOD_TIMEUNIT_PCMBYTES) {
        return FMOD_ERR_FORMAT;
    } else if (this->mDSPCodec) {
        channels = this->mDSPCodec->mDescription.channels;
        format = this->mDSPCodec->mDescription.mFormat;
        frequency = this->mDSPCodec->mDefaultFrequency;
    } else if (this->mSound) {
        channels = this->mSound->mChannels;
        format = this->mSound->mFormat;
        frequency = this->mSound->mDefaultFrequency;
    } else if (this->mDSPResampler) {
        channels = this->mDSPResampler->mDescription.channels;
        format = FMOD_SOUND_FORMAT_PCMFLOAT;
        frequency = this->mDSPResampler->mDefaultFrequency;
    } else {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (postype == FMOD_TIMEUNIT_PCM) {
        pcm = position;
    } else if (postype == FMOD_TIMEUNIT_PCMBYTES) {
        SoundI::getSamplesFromBytes(position, &pcm, channels, format);
    } else if (postype == FMOD_TIMEUNIT_MS) {
        pcm = static_cast<uint32_t>(static_cast<float>(position) / 1000.0f * frequency);
    }

    if (this->mSound) {
        if (this->mMode & FMOD_LOOP_OFF) {
            endpoint = this->mSound->mLength - 1;
        } else {
            endpoint = this->mLoopStart + this->mLoopLength - 1;
        }
    } else {
        endpoint = static_cast<uint32_t>(-1);
    }

    if (pcm > endpoint) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mDSPCodec) {
        return this->mDSPCodec->setPosition(pcm, false);
    } else if (this->mDSPWaveTable) {
        return this->mDSPWaveTable->setPosition(pcm, false);
    } else if (this->mDSPResampler) {
        return this->mDSPResampler->setPosition(pcm, true);
    } else {
        return this->mDSPHead->setPosition(pcm, true);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::getPosition(uint32_t* position, FMOD_TIMEUNIT postype) {
    int32_t channels;
    FMOD_SOUND_FORMAT format;
    float frequency;

    if (!position) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (postype != FMOD_TIMEUNIT_MS && postype != FMOD_TIMEUNIT_PCM && postype != FMOD_TIMEUNIT_PCMBYTES) {
        return FMOD_ERR_FORMAT;
    }

    if (this->mDSPCodec) {
        auto dspcodec = static_cast<DSPResampler*>(this->mDSPCodec);

        channels = dspcodec->mDescription.channels;
        frequency = this->mSound->mDefaultFrequency;
        this->mPosition = dspcodec->mPosition.mHi;
    } else if (this->mSound) {
        if (!this->mDSPWaveTable) {
            return FMOD_ERR_INVALID_PARAM;
        }

        frequency = this->mSound->mDefaultFrequency;
        channels = this->mSound->mChannels;
        format = this->mSound->mFormat;

        this->mPosition = this->mDSPWaveTable->mPosition.mHi;
    } else if (this->mDSPResampler) {
        channels = this->mDSPResampler->mDescription.channels;
        format = FMOD_SOUND_FORMAT_PCMFLOAT;
        frequency = this->mDSPResampler->mDefaultFrequency;
    }

    auto pcmcurrent = this->mPosition;

    if (postype == FMOD_TIMEUNIT_PCM) {
        *position = pcmcurrent;
    } else if (postype == FMOD_TIMEUNIT_PCMBYTES) {
        SoundI::getBytesFromSamples(pcmcurrent, position, channels, format);
    } else if (postype == FMOD_TIMEUNIT_MS) {
        *position = static_cast<uint32_t>((static_cast<float>(pcmcurrent) / frequency) * 1000.0f);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setLoopPoints(uint32_t loopstart, uint32_t looplength) {
    FMOD_RESULT result;

    result = ChannelReal::setLoopPoints(loopstart, looplength);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDSPCodec) {
        this->mDSPCodec->mLoopStart = loopstart;
        this->mDSPCodec->mLoopLength = looplength;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setLoopCount(int32_t loopcount) {
    FMOD_RESULT result;

    result = ChannelReal::setLoopCount(loopcount);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDSPCodec) {
        this->mDSPCodec->mLoopCount = loopcount;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::set3DOcclusion(float directOcclusion, float reverbOcclusion) {
    FMOD_RESULT result;

    if (this->mSubChannelIndex > 0) {
        return FMOD_OK;
    }

    this->mParent->mReverbOcclusion = reverbOcclusion;
    this->mParent->mDirectOcclusion = directOcclusion;
    this->mParent->mDirectOcclusionVolume = 1.0f - directOcclusion;

    result = this->updateDirectMix(this->mParent->mVolume);
    if (result != FMOD_OK) {
        return result;
    }

    auto reverb = static_cast<ReverbI*>(this->mSystem->mReverb3DHead.getNext());
    while (reverb != &this->mSystem->mReverb3DHead) {
        if (reverb->getMode() == FMOD_REVERB_PHYSICAL) {
            result = this->updateReverbMix(reverb, this->mParent->mVolume);
            if (result != FMOD_OK) {
                return result;
            }
        }

        reverb = static_cast<ReverbI*>(reverb->getNext());
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop) {
    FMOD_RESULT result;
    DSPConnection* connection;
    float lindirect;

    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    lindirect = FMOD_POW(10.0f, prop->Direct / 2000.0f);

    if (lindirect != this->mParent->mReverbDryVolume) {
        this->mParent->mReverbDryVolume = lindirect;
        this->updateDirectMix(this->mParent->mVolume);
    }

    result = this->mSystem->mReverbGlobal.setChanProperties(
        this->mParent->mIndex,
        prop,
        nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mSystem->mReverbGlobal.mDSP) {
        this->mSystem->mReverbGlobal.getChanProperties(
            this->mParent->mIndex,
            nullptr,
            &connection);

        if (!connection) {
            if (this->mDSPCodec) {
                this->mDSPReverb = this->mDSPCodec;
            } else if (this->mDSPResampler) {
                this->mDSPReverb = this->mDSPResampler;
            } else {
                this->mDSPReverb = this->mDSPWaveTable;
            }

            this->addToReverbs(this->mDSPReverb);
        }

        result = this->updateReverbMix(
            &this->mSystem->mReverbGlobal,
            this->mParent->mVolume);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->mSystem->mReverb3D.setChanProperties(
        this->mParent->mIndex,
        prop,
        nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mSystem->mReverb3D.mDSP) {
        this->mSystem->mReverb3D.getChanProperties(
            this->mParent->mIndex,
            nullptr,
            &connection);

        if (!connection) {
            if (this->mDSPCodec) {
                this->mDSPReverb = this->mDSPCodec;
            } else if (this->mDSPResampler) {
                this->mDSPReverb = this->mDSPResampler;
            } else {
                this->mDSPReverb = this->mDSPWaveTable;
            }

            this->addToReverbs(this->mDSPReverb);
        }

        result = this->updateReverbMix(
            &this->mSystem->mReverb3D,
            this->mParent->mVolume);
        if (result != FMOD_OK) {
            return result;
        }
    }

    auto reverb = static_cast<ReverbI*>(this->mSystem->mReverb3DHead.getNext());
    while (reverb != &this->mSystem->mReverb3DHead) {
        if (reverb->getMode() == FMOD_REVERB_PHYSICAL) {
            reverb->setChanProperties(
                this->mParent->mIndex,
                prop,
                nullptr);
            if (reverb->mDSP) {
                reverb->getChanProperties(
                    this->mParent->mIndex,
                    nullptr,
                    &connection);
                if (!connection) {
                    if (this->mDSPCodec) {
                        this->mDSPReverb = this->mDSPCodec;
                    } else if (this->mDSPResampler) {
                        this->mDSPReverb = this->mDSPResampler;
                    } else {
                        this->mDSPReverb = this->mDSPWaveTable;
                    }

                    this->addToReverbs(this->mDSPReverb);
                }

                result = this->updateReverbMix(reverb, this->mParent->mVolume);
                if (result != FMOD_OK) {
                    return result;
                }
            }
        }

        reverb = static_cast<ReverbI*>(reverb->getNext());
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop) {
    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    return this->mSystem->mReverbGlobal.getChanProperties(
        this->mParent->mIndex,
        prop,
        nullptr);
}

FMOD_RESULT ChannelSoftware::isPlaying(bool* isplaying, bool includethreadlatency) {
    FMOD_RESULT result;

    if (!isplaying) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mFlags & CHANNELREAL_FLAG_ALLOCATED) {
        *isplaying = true;
    } else {
        if (this->mDSPCodec) {
            this->mDSPCodec->getFinished(isplaying);
            *isplaying = !*isplaying;

            if (*isplaying) {
                return FMOD_OK;
            }

        } else if (this->mDSPCodecStopped && includethreadlatency) {
            this->mDSPCodecStopped->getFinished(isplaying);
            *isplaying = !*isplaying;

            if (*isplaying) {
                return FMOD_OK;
            }

            this->mDSPCodecStopped = nullptr;
        } else if (this->mDSPResampler) {
            DSPI* input;
            int32_t numfinished;
            int32_t numinputs;
            result = this->mDSPResampler->getNumInputs(&numinputs);
            if (result == FMOD_OK) {
                for (int32_t i = 0; i < numinputs; i++) {
                    result = this->mDSPResampler->getInput(i, &input);
                    if (result != FMOD_OK) {
                        break;
                    }

                    bool finished;
                    input->getFinished(&finished);
                    if (finished) {
                        numfinished++;
                    }
                }

                if (numinputs != numfinished) {
                    *isplaying = true;
                    return FMOD_OK;
                }
            }

            *isplaying = false;
        } else if (this->mSound || this->mDSP) {
            this->mDSPWaveTable->getFinished(isplaying);
            *isplaying = !*isplaying;

            if (*isplaying) {
                return FMOD_OK;
            }
        }
    }

    if (this->mDelayEnd) {
        this->mFlags |= CHANNELREAL_FLAG_ENDDELAY;
        *isplaying = true;
        return FMOD_OK;
    }

    this->mFlags &= ~(CHANNELREAL_FLAG_ALLOCATED | CHANNELREAL_FLAG_PLAYING);

    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::getSpectrum(
    float* spectrumarray,
    int32_t numvalues,
    int32_t channeloffset,
    FMOD_DSP_FFT_WINDOW windowtype) {

    FMOD_RESULT result;

    DSPFilter* dsphead;
    float* buffer;
    uint32_t position;
    uint32_t length;
    uint32_t bufferlength;
    int32_t numchannels;
    int32_t windowsize;
    static DSPFFT fft;

    dsphead = static_cast<DSPFilter*>(this->mDSPHead);
    if (!dsphead) {
        return FMOD_ERR_INITIALIZATION;
    }

    windowsize = numvalues * 2;

    if (windowsize != (1 << 7) &&
        windowsize != (1 << 8) &&
        windowsize != (1 << 9) &&
        windowsize != (1 << 10) &&
        windowsize != (1 << 11) &&
        windowsize != (1 << 12) &&
        windowsize != (1 << 13) &&
        windowsize != (1 << 14)) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mSound) {
        numchannels = this->mSound->mChannels;
    } else if (this->mDSP) {
        numchannels = this->mDSP->mDescription.channels;
    } else {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (channeloffset >= numchannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = dsphead->startBuffering(16384);
    if (result != FMOD_OK) {
        return result;
    }

    result = dsphead->getHistoryBuffer(&buffer, &position, &length);
    if (result != FMOD_OK) {
        return result;
    }

    if (windowsize > static_cast<int32_t>(length)) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->mSystem->getDSPBufferSize(&bufferlength, 0);

    position -= windowsize;
    if (static_cast<int32_t>(position) < 0) {
        position += length;
    }

    result = fft.getSpectrum(
        buffer,
        position,
        length,
        spectrumarray,
        windowsize,
        channeloffset,
        numchannels,
        windowtype);
    return result;
}

FMOD_RESULT ChannelSoftware::getWaveData(
    float* wavearray,
    int32_t numvalues,
    int32_t channeloffset) {

    FMOD_RESULT result;

    DSPFilter* dsphead;
    float* buffer;
    uint32_t position;
    uint32_t length;
    int32_t numchannels;

    dsphead = reinterpret_cast<DSPFilter*>(this->mDSPHead);
    if (!dsphead) {
        return FMOD_ERR_INITIALIZATION;
    }

    if (this->mSound) {
        numchannels = this->mSound->mChannels;
    } else if (this->mDSP) {
        numchannels = this->mDSP->mDescription.channels;
    } else {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (channeloffset >= numchannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = dsphead->startBuffering(16384);
    if (result != FMOD_OK) {
        return result;
    }

    result = dsphead->getHistoryBuffer(&buffer, &position, &length);
    if (result != FMOD_OK) {
        return result;
    }

    if (numvalues > static_cast<int32_t>(length)) {
        return FMOD_ERR_INVALID_PARAM;
    }

    position -= numvalues;
    if (static_cast<int32_t>(position) < 0) {
        position += length;
    }

    for (int32_t i = 0; i < numvalues; i++) {
        wavearray[i] = buffer[position * numchannels + channeloffset];
        position++;
        if (position >= length) {
            position = 0;
        }
    }

    return result;
}

FMOD_RESULT ChannelSoftware::getDSPHead(DSPI** dsp) {
    *dsp = this->mDSPHead;
    return FMOD_OK;
}

FMOD_RESULT ChannelSoftware::setMode(FMOD_MODE mode) {
    FMOD_RESULT result;

    result = ChannelReal::setMode(mode);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDSPCodec) {
        this->mDSPCodec->mMode = this->mMode;
    }

    return FMOD_OK;
}

} // namespace FMOD
