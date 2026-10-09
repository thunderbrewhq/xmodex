#include "xmodex/channel/ChannelEmulated.hpp"
#include "fmod.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelStream.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/sound/SoundI.hpp"
#include "xmodex/sound/Stream.hpp"

namespace FMOD {

ChannelEmulated::ChannelEmulated() {
    this->mDSPHead = nullptr;
}

FMOD_RESULT ChannelEmulated::init(int32_t index, SystemI* system, Output* output, DSPI* dspmixtarget) {
    ChannelReal::init(index, system, output, dspmixtarget);

    if (!(this->mSystem->mFlags & FMOD_INIT_DISABLESOFTWARE)) {
        FMOD_DSP_DESCRIPTION_EX descriptionex;
        FMOD_RESULT result;

        FMOD_memset(&descriptionex, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));
        FMOD_strcpy(descriptionex.name, "EmulatedChannel DSPHead Unit");

        descriptionex.version = 0x10100;
        descriptionex.mCategory = FMOD_DSP_CATEGORY_FILTER;
        descriptionex.mFormat = FMOD_SOUND_FORMAT_PCMFLOAT;

        result = this->mSystem->createDSP(&descriptionex, &this->mDSPHead, false);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelEmulated::close() {
    FMOD_RESULT result;

    result = ChannelReal::close();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDSPHead) {
        this->mDSPHead->release(false);
        this->mDSPHead = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelEmulated::alloc() {
    FMOD_RESULT result;

    result = ChannelReal::alloc();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDSPHead) {
        result = this->mDSPHead->disconnectFrom(nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mParent->mChannelGroup->mDSPHead->addInputQueued(this->mDSPHead, false, nullptr, nullptr);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return result;
}

FMOD_RESULT ChannelEmulated::update(int32_t delta) {
    FMOD_RESULT result;

    result = ChannelReal::update(delta);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mFlags & CHANNELREAL_FLAG_PAUSED || this->mFlags & CHANNELREAL_FLAG_ENDDELAY || !(this->mFlags & CHANNELREAL_FLAG_PLAYING)) {
        return FMOD_OK;
    }

    delta = (delta * static_cast<int32_t>(this->mParent->mFrequency * this->mParent->mPitch3D)) / 1000;

    if (this->mDirection == CHANNELREAL_PLAYDIR_BACKWARDS) {
        delta = -delta;
    }

    this->mPosition += delta;

    if (this->mSound) {
        if (this->mMode & FMOD_LOOP_NORMAL || (this->mMode & FMOD_LOOP_BIDI && this->mLoopCount)) {
            while ((this->mPosition >= this->mLoopStart + this->mLoopLength) ||
                   (this->mPosition <= this->mLoopStart)) {
                if (!this->mLoopCount) {
                    if (this->mDirection == CHANNELREAL_PLAYDIR_FORWARDS) {
                        this->mPosition = this->mLoopStart + this->mLoopLength;
                    } else {
                        this->mPosition = this->mLoopStart;
                    }

                    this->mFlags &= ~CHANNELREAL_FLAG_PLAYING;
                    break;
                } else if (this->mMode & FMOD_LOOP_NORMAL) {
                    this->mPosition -= this->mLoopLength;
                } else if (this->mMode & FMOD_LOOP_BIDI) {
                    if (this->mDirection == CHANNELREAL_PLAYDIR_FORWARDS) {
                        this->mDirection = CHANNELREAL_PLAYDIR_BACKWARDS;
                    } else {
                        this->mDirection = CHANNELREAL_PLAYDIR_FORWARDS;
                    }
                    this->mPosition += -delta;
                }

                if (this->mLoopCount >= 0) {
                    this->mLoopCount--;
                }
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelEmulated::stop(bool ignoreenddelay, bool resetstate, bool force) {
    if (!(this->mFlags & CHANNELREAL_FLAG_ENDDELAY) || ignoreenddelay) {
        if (this->mSound && this->mSound->isStream()) {
            auto stream = static_cast<Stream*>(this->mSound);
            if (stream->mChannel) {
                stream->mChannel->mFinished = true;
            }
        }

        if (this->mDSPHead) {
            this->mDSPHead->setActive(false);
            this->mDSPHead->disconnectAll(false, true);

            this->mDSPHead->mFlags &= ~FMOD_DSP_FLAG_VISITED;
            this->mDSPHead->mFlags |= FMOD_DSP_FLAG_FINISHED;
            *this->mDSPHead->mWantsToFinish = 0;
        }

        ChannelReal::stop(ignoreenddelay, true);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelEmulated::isVirtual(bool* isvirtual) {
    if (!isvirtual) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *isvirtual = true;

    return FMOD_OK;
}

FMOD_RESULT ChannelEmulated::getDSPHead(DSPI** dsp) {
    if (dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *dsp = this->mDSPHead;

    return FMOD_OK;
}

} // namespace FMOD
