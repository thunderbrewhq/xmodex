#include "xmodex/channel/ChannelReal.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelI.hpp"
#include "xmodex/channel/ChannelPool.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

ChannelReal::ChannelReal() {
    for (int32_t i = 0; i < 16; i++) {
        this->mSubChannel[i] = nullptr;
    }

    this->mMinFrequency = 100.0f;
    this->mSound = nullptr;
    this->mMaxFrequency = 1000000.0f;
    this->mSystem = nullptr;
    this->mOutput = nullptr;
    this->mPool = nullptr;
    this->mLoopCount = -1;
    this->mNumSubChannels = 1;
}

ChannelReal::~ChannelReal() {
}

FMOD_RESULT ChannelReal::init(int32_t index, SystemI* system, Output* output, DSPI* dspmixtarget) {
    this->mSound = nullptr;
    this->mFlags = 0;
    this->mMode = 0;
    this->mPosition = 0;
    this->mDirection = 0;
    this->mDelayStart = 0;
    this->mDelayEnd = 0;
    this->mOutput = output;
    this->mIndex = index;
    this->mLoopCount = -1;
    this->mSystem = system;
    this->mNumSubChannels = 1;
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::set2DFreqVolumePanFor3D() {
    return FMOD_OK;
}

bool ChannelReal::isStream() {
    return false;
}

FMOD_RESULT ChannelReal::moveChannelGroup(ChannelGroupI* oldchannelgroup, ChannelGroupI* newchannelgroup) {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::close() {
    return this->stop(true, true);
}

FMOD_RESULT ChannelReal::alloc(DSPI* dsp) {
    if (this->mPool) {
        this->mPool->mChannelsUsed++;
    }

    this->mPosition = 0;
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::alloc() {
    this->mPosition = 0;

    if (this->mPool) {
        this->mPool->mChannelsUsed++;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::start() {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::update(int32_t delta) {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::updateStream() {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::stop(bool ignoreenddelay, bool resetstate, bool force) {
    return this->stop(ignoreenddelay, resetstate);
}

FMOD_RESULT ChannelReal::stop(bool ignoreenddelay, bool resetstate) {
    if (!ignoreenddelay && this->mDelayEnd) {
        this->mFlags |= CHANNELREAL_FLAG_ENDDELAY;
        return FMOD_OK;
    }

    if (this->mPool) {
        this->mPool->mChannelsUsed--;
    }

    if (resetstate) {
        this->mFlags &= ~(CHANNELREAL_FLAG_ALLOCATED |
                          CHANNELREAL_FLAG_PAUSED |
                          CHANNELREAL_FLAG_PLAYING |
                          CHANNELREAL_FLAG_PAUSEDFORSETPOS |
                          CHANNELREAL_FLAG_ENDDELAY);
        this->mFlags |= CHANNELREAL_FLAG_STOPPED;
    }

    this->mDelayStart = 0;
    this->mDelayEnd = 0;

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setPaused(bool paused) {
    if (paused) {
        this->mFlags |= CHANNELREAL_FLAG_PAUSED;
    } else {
        this->mFlags &= ~CHANNELREAL_FLAG_PAUSED;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::getPaused(bool* paused) {
    if (!paused) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *paused = this->mFlags & CHANNELREAL_FLAG_PAUSED ? true : false;

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setVolume(float volume) {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setFrequency(float frequency) {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setPan(float pan, float fbpan) {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setDelay(uint32_t startdelay, uint32_t enddelay) {
    this->mDelayStart = startdelay;
    this->mDelayEnd = enddelay;

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setSpeakerMix(
    float frontleft,
    float frontright,
    float center,
    float lfe,
    float backleft,
    float backright,
    float sideleft,
    float sideright) {

    int32_t numsubchannels = 0;
    if (this->mSound) {
        if (this->mSound->mSubSampleParent) {
            numsubchannels = this->mSound->mSubSampleParent->mChannels;
        } else {
            numsubchannels = this->mSound->mChannels;
        }
    } else if (this->mDSP) {
        numsubchannels = this->mDSP->mDescription.channels;
    }

    if (numsubchannels > 1) {
        switch (this->mSubChannelIndex) {
        case 0: {
            this->setPan(-1.0f, 1.0f);
            this->setVolume(frontleft);
            break;
        }
        case 1: {
            this->setPan(1.0f, 1.0f);
            this->setVolume(frontright);
            break;
        }
        case 2: {
            this->setPan(-1.0f, 1.0f);
            this->setVolume(center);
            break;
        }
        case 3: {
            this->setPan(1.0f, 1.0f);
            this->setVolume(lfe);
            break;
        }
        case 4: {
            this->setPan(-1.0f, 1.0f);
            this->setVolume(backleft);
            break;
        }
        case 5: {
            this->setPan(1.0f, 1.0f);
            this->setVolume(backright);
            break;
        }
        case 6: {
            this->setPan(-1.0f, 1.0f);
            this->setVolume(sideleft);
            break;
        }
        case 7: {
            this->setPan(1.0f, 1.0f);
            this->setVolume(sideright);
            break;
        }
        default: {
            break;
        }
        }
    }

    float vol = 0.0f;
    float lrpan = 0.0f;
    float fbpan = 0.0f;

    vol += frontleft;
    vol += frontright;
    vol += center;
    vol += lfe;
    vol += backleft;
    vol += backright;
    vol += sideleft;
    vol += sideright;
    vol = vol > 1.0f ? 1.0f : vol;

    lrpan -= frontleft;
    lrpan += frontright;
    lrpan -= backleft;
    lrpan += backright;
    lrpan -= sideleft;
    lrpan += sideright;

    fbpan += frontleft;
    fbpan += frontright;
    fbpan += center;
    fbpan -= backleft;
    fbpan -= backright;

    this->setVolume(vol * this->mParent->mVolume);
    this->setPan(lrpan < -1.0f ? -1.0f : lrpan > 1.0f ? 1.0f
                                                      : lrpan,
                 fbpan < -1.0f ? -1.0f : fbpan > 1.0f ? 1.0f
                                                      : fbpan);
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setSpeakerLevels(int32_t speaker, float* levels, int32_t numlevels) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mParent) {
        result = this->updateSpeakerLevels(this->mParent->mVolume);
    }

    return result;
}

FMOD_RESULT ChannelReal::updateSpeakerLevels(float volume) {
    if (this->mParent && this->mParent->mLevels) {
        auto outputchannels = this->mSystem->mMaxOutputChannels;

        float pan = 0.0f;
        float vol = 0.0f;

        for (int32_t i = 0; i < outputchannels; i++) {
            float val = FMOD_FABS(this->mParent->mLevels[(i * this->mSystem->mMaxInputChannels) + this->mSubChannelIndex]);

            vol += val;

            if (i == FMOD_SPEAKER_FRONT_LEFT || i == FMOD_SPEAKER_BACK_LEFT || i == FMOD_SPEAKER_SIDE_LEFT) {
                pan -= val;
            } else if (i == FMOD_SPEAKER_FRONT_RIGHT || i == FMOD_SPEAKER_BACK_RIGHT || i == FMOD_SPEAKER_SIDE_RIGHT) {
                pan += val;
            }
        }

        vol = vol > 1.0f ? 1.0f : vol;
        this->setVolume(vol);
        this->setPan(pan < -1.0f ? -1.0f : pan > 1.0f ? 1.0f
                                                      : pan,
                     0.0f);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setPosition(uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;

    if (postype != FMOD_TIMEUNIT_MS &&
        postype != FMOD_TIMEUNIT_PCM &&
        postype != FMOD_TIMEUNIT_PCMBYTES) {
        return FMOD_ERR_FORMAT;
    }

    if (this->mSound) {
        uint32_t lengthpcm;
        uint32_t pcm;

        result = this->mSound->getLength(&lengthpcm, FMOD_TIMEUNIT_PCM);
        if (result != FMOD_OK) {
            return result;
        }

        if (postype == FMOD_TIMEUNIT_PCM) {
            pcm = position;
        } else if (postype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getSamplesFromBytes(position, &pcm, this->mSound->mChannels, this->mSound->mFormat);
        } else if (postype == FMOD_TIMEUNIT_MS) {
            pcm = static_cast<uint32_t>(static_cast<float>(position) / 1000.0f * this->mSound->mDefaultFrequency);
        }

        if (pcm >= lengthpcm) {
            pcm = lengthpcm;
        }

        this->mPosition = pcm;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::getPosition(uint32_t* position, FMOD_TIMEUNIT postype) {
    bool getsubsoundtime = false;

    if (!position) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mSound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    postype = static_cast<FMOD_TIMEUNIT>(postype & ~FMOD_TIMEUNIT_BUFFERED);

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

        int32_t currentsubsoundid = 0;
        int32_t currentsentenceid = 0;
        uint32_t pcmcurrent = this->mPosition;

        if (getsubsoundtime) {
            for (currentsubsoundid = 0; currentsubsoundid < this->mSound->mSubSoundListNum; currentsubsoundid++) {
                auto sound = this->mSound->mSubSound[this->mSound->mSubSoundList[currentsubsoundid]];

                if (sound && pcmcurrent >= sound->mLength) {
                    pcmcurrent -= sound->mLength;
                    currentsentenceid++;
                } else {
                    break;
                }
            }
        }

        if (postype == FMOD_TIMEUNIT_SENTENCE) {
            *position = currentsentenceid;
        } else if (postype == FMOD_TIMEUNIT_SENTENCE_SUBSOUND) {
            *position = currentsubsoundid;
        } else if (postype == FMOD_TIMEUNIT_PCM) {
            *position = pcmcurrent;
        } else if (postype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getBytesFromSamples(pcmcurrent, position, this->mSound->mChannels, this->mSound->mFormat);
        } else if (postype == FMOD_TIMEUNIT_MS) {
            *position = static_cast<uint32_t>(static_cast<float>(pcmcurrent) / this->mSound->mDefaultFrequency * 1000.0f);
        }
    } else {
        return FMOD_ERR_FORMAT;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setLoopPoints(uint32_t loopstart, uint32_t looplength) {
    if (!this->mSound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (loopstart >= this->mSound->mLength) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (loopstart + looplength > this->mSound->mLength) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mLoopStart = loopstart;
    this->mLoopLength = looplength;

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setLoopCount(int32_t loopcount) {
    this->mLoopCount = loopcount;
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::set3DAttributes() {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::set3DMinMaxDistance() {
    return FMOD_OK;
}

FMOD_RESULT ChannelReal::set3DOcclusion(float directOcclusion, float reverbOcclusion) {
    if (this->mParent) {
        return this->setVolume(this->mParent->mVolume);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop) {
    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mParent) {
        return this->mSystem->mReverbGlobal.setChanProperties(
            this->mParent->mIndex,
            prop,
            nullptr);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop) {
    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mParent) {
        return this->mSystem->mReverbGlobal.getChanProperties(
            this->mParent->mIndex,
            prop,
            nullptr);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::isPlaying(bool* isplaying, bool includethreadlatency) {
    if (!isplaying) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mFlags & (CHANNELREAL_FLAG_ALLOCATED | CHANNELREAL_FLAG_PLAYING) || this->mFlags & CHANNELREAL_FLAG_ENDDELAY) {
        *isplaying = true;
    } else {
        *isplaying = false;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::isVirtual(bool* isvirtual) {
    if (!isvirtual) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *isvirtual = false;

    return FMOD_OK;
}

FMOD_RESULT ChannelReal::getSpectrum(
    float* spectrumarray,
    int32_t numentries,
    int32_t channeloffset,
    FMOD_DSP_FFT_WINDOW windowtype) {
    return FMOD_ERR_NEEDSSOFTWARE;
}

FMOD_RESULT ChannelReal::getWaveData(float* wavearray, int32_t numvalues, int32_t channeloffset) {
    return FMOD_ERR_NEEDSSOFTWARE;
}

FMOD_RESULT ChannelReal::getDSPHead(DSPI** dsp) {
    *dsp = nullptr;
    return FMOD_ERR_DSP_NOTFOUND;
}

FMOD_RESULT ChannelReal::setMode(FMOD_MODE mode) {
    if (mode & (FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI)) {
        this->mMode = this->mMode & ~(FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI);

        if (mode & FMOD_LOOP_OFF) {
            this->mMode |= FMOD_LOOP_OFF;
        } else if (mode & FMOD_LOOP_NORMAL) {
            this->mMode |= FMOD_LOOP_NORMAL;
        } else if (mode & FMOD_LOOP_BIDI) {
            this->mMode |= FMOD_LOOP_BIDI;
        }
    }

    if (mode & FMOD_3D_HEADRELATIVE) {
        this->mMode &= ~FMOD_3D_WORLDRELATIVE;
        this->mMode |= FMOD_3D_HEADRELATIVE;
    } else if (mode & FMOD_3D_WORLDRELATIVE) {
        this->mMode &= ~FMOD_3D_HEADRELATIVE;
        this->mMode |= FMOD_3D_WORLDRELATIVE;
    }

    if (mode & FMOD_3D_LOGROLLOFF) {
        this->mMode &= ~FMOD_3D_LINEARROLLOFF;
        this->mMode &= ~FMOD_3D_CUSTOMROLLOFF;
        this->mMode |= FMOD_3D_LOGROLLOFF;
    } else if (mode & FMOD_3D_LINEARROLLOFF) {
        this->mMode &= ~FMOD_3D_LOGROLLOFF;
        this->mMode &= ~FMOD_3D_CUSTOMROLLOFF;
        this->mMode |= FMOD_3D_LINEARROLLOFF;
    } else if (mode & FMOD_3D_CUSTOMROLLOFF) {
        this->mMode &= ~FMOD_3D_LOGROLLOFF;
        this->mMode &= ~FMOD_3D_LINEARROLLOFF;
        this->mMode |= FMOD_3D_CUSTOMROLLOFF;
    }

    if (mode & FMOD_3D_IGNOREGEOMETRY) {
        this->mMode |= FMOD_3D_IGNOREGEOMETRY;
    } else {
        this->mMode &= ~FMOD_3D_IGNOREGEOMETRY;
    }

    if (mode & FMOD_VIRTUAL_PLAYFROMSTART) {
        this->mMode |= FMOD_VIRTUAL_PLAYFROMSTART;
    } else {
        this->mMode &= ~FMOD_VIRTUAL_PLAYFROMSTART;
    }

    if (!(this->mMode & FMOD_HARDWARE)) {
        if (mode & FMOD_2D) {
            if (!this->mParent) {
                return FMOD_OK;
            }

            this->mMode &= ~FMOD_3D;
            this->mMode |= FMOD_2D;
            this->mParent->mVolume3D = 1.0f;
            this->mParent->mDirectOcclusionVolume = 1.0f;
            this->mParent->mReverbDryVolume = 1.0f;
            this->mParent->mConeVolume3D = 1.0f;
            this->mParent->mPitch3D = 1.0f;
        } else if (mode & FMOD_3D) {
            this->mMode &= ~FMOD_2D;
            this->mMode |= FMOD_3D;
        }
    }

    return FMOD_OK;
}

} // namespace FMOD
