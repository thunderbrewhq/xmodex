#include "xmodex/channel/ChannelGroupI.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelI.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/output/Output.hpp"

namespace FMOD {

ChannelGroupI::ChannelGroupI() {
    this->mRealVolume = 1.0f;
    this->mVolume = 1.0f;
    this->mRealPitch = 1.0f;
    this->mPitch = 1.0f;
    this->mReverbOcclusion = 0.0f;
    this->mDirectOcclusion = 0.0f;
    this->mRealReverbOcclusionVolume = 1.0f;
    this->mRealDirectOcclusionVolume = 1.0f;
}

FMOD_RESULT ChannelGroupI::setVolumeInternal() {
    float newvolume;
    bool forceupdatepos = false;
    float v;

    newvolume = this->mParent ? this->mParent->mRealVolume : 1.0f;

    newvolume *= this->mVolume;

    if (this->mRealVolume != newvolume) {
        forceupdatepos = true;
    }

    mRealVolume = newvolume;

    if (this->mGroupHead) {
        auto currentgroup = reinterpret_cast<ChannelGroupI*>(this->mGroupHead->getNext());
        while (currentgroup != this->mGroupHead) {
            currentgroup->setVolumeInternal();
            currentgroup = reinterpret_cast<ChannelGroupI*>(currentgroup->getNext());
        }
    }

    auto current = this->mChannelHead.getNext();
    while (current != &this->mChannelHead) {
        auto channel = reinterpret_cast<ChannelI*>(current->getData());
        channel->getVolume(&v);
        channel->setVolume(v, forceupdatepos);
        current = current->getNext();
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::setPitchInternal() {
    float freq;

    this->mRealPitch = (this->mParent ? this->mParent->mRealPitch : 1.0f) * this->mPitch;

    if (this->mGroupHead) {
        auto currentgroup = reinterpret_cast<ChannelGroupI*>(this->mGroupHead->getNext());
        while (currentgroup != this->mGroupHead) {
            currentgroup->setPitchInternal();
            currentgroup = reinterpret_cast<ChannelGroupI*>(currentgroup->getNext());
        }
    }

    auto current = this->mChannelHead.getNext();
    while (current != &this->mChannelHead) {
        auto channel = reinterpret_cast<ChannelI*>(current->getData());
        channel->getFrequency(&freq);
        channel->setFrequency(freq);
        current = current->getNext();
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::setPaused(bool paused, bool setpausedflag) {
    if (setpausedflag) {
        this->mPaused = paused;
    }

    if (this->mGroupHead) {
        auto currentgroup = reinterpret_cast<ChannelGroupI*>(this->mGroupHead->getNext());
        while (currentgroup != this->mGroupHead) {
            currentgroup->setPaused(paused, false);
            currentgroup = reinterpret_cast<ChannelGroupI*>(currentgroup->getNext());
        }
    }

    auto current = this->mChannelHead.getNext();
    while (current != &this->mChannelHead) {
        auto channel = reinterpret_cast<ChannelI*>(current->getData());
        channel->setPaused(channel->mFlags & CHANNELI_FLAG_PAUSED ? true : false);
        current = current->getNext();
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::setMute(bool mute, bool setmuteflag) {
    if (setmuteflag) {
        this->mMute = mute;
    }

    if (this->mGroupHead) {
        auto currentgroup = reinterpret_cast<ChannelGroupI*>(this->mGroupHead->getNext());
        while (currentgroup != this->mGroupHead) {
            currentgroup->setMute(mute, false);
            currentgroup = reinterpret_cast<ChannelGroupI*>(currentgroup->getNext());
        }
    }

    auto current = this->mChannelHead.getNext();
    while (current != &this->mChannelHead) {
        auto channel = reinterpret_cast<ChannelI*>(current->getData());
        channel->setMute(channel->mFlags & CHANNELI_FLAG_MUTED ? true : false);
        current = current->getNext();
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::getDSPHead(DSPI** dsp) {
    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *dsp = this->mDSPHead;
    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::getNumChannels(int32_t* numchannels) {
    if (!numchannels) {
        return FMOD_ERR_INVALID_PARAM;
    }
    *numchannels = this->mNumChannels;
    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::setVolume(float volume) {
    if (volume < 0.0f) {
        volume = 0.0f;
    }
    if (volume > 1.0f) {
        volume = 1.0f;
    }
    this->mVolume = volume;

    return this->setVolumeInternal();
}

FMOD_RESULT ChannelGroupI::addGroup(ChannelGroupI* group) {
    FMOD_RESULT result;

    if (!group) {
        return FMOD_ERR_INVALID_PARAM;
    }

    group->removeNode();

    if (group->mDSPHead) {
        group->mDSPHead->disconnectAllQueued(false, true);
    }

    if (!this->mGroupHead) {
        this->mGroupHead = FMOD_Object_Calloc(ChannelGroupI);
        if (!this->mGroupHead) {
            return FMOD_ERR_MEMORY;
        }
    }

    group->addBefore(this->mGroupHead);

    if (this->mDSPMixTarget) {
        result = this->mDSPMixTarget->addInputQueued(group->mDSPHead, false, nullptr, nullptr);
        if (result != FMOD_OK) {
            return result;
        }
    }

    group->mParent = this;

    result = group->setPaused(this->mPaused, true);
    if (result != FMOD_OK) {
        return result;
    }

    result = group->setMute(this->mMute, true);
    if (result != FMOD_OK) {
        return result;
    }

    result = group->setVolumeInternal();
    if (result != FMOD_OK) {
        return result;
    }

    result = group->setPitchInternal();
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::releaseInternal() {
    ChannelGroupI* mastergroup;

    if (this->mSystem->mChannelGroup && this != this->mSystem->mChannelGroup) {
        auto current = this->mChannelHead.getNext();
        while (current != &this->mChannelHead) {
            auto channel = reinterpret_cast<ChannelI*>(current->getData());
            channel->setChannelGroup(this->mSystem->mChannelGroup);
            current = current->getNext();
        }
    }

    if (this->mDSPMixTarget && this->mDSPMixTarget != this->mDSPHead) {
        this->mDSPMixTarget->release(true);
        this->mDSPMixTarget = nullptr;
    }

    if (this->mDSPHead) {
        this->mDSPHead->release(true);
        this->mDSPHead = nullptr;
    }

    if (this->mName) {
        FMOD_Memory_Free(this->mName);
    }

    if (this->mGroupHead) {
        auto currentgroup = reinterpret_cast<ChannelGroupI*>(this->mGroupHead->getNext());

        mSystem->getMasterChannelGroup(&mastergroup);

        if (mastergroup && this != mastergroup) {
            while (currentgroup != this->mGroupHead) {
                mastergroup->addGroup(currentgroup);
                currentgroup = reinterpret_cast<ChannelGroupI*>(currentgroup->getNext());
            }
        }

        FMOD_Memory_Free(this->mGroupHead);
    }

    if (this == this->mSystem->mOutput->mMusicChannelGroup) {
        this->mSystem->mOutput->mMusicChannelGroup = nullptr;
    }

    this->removeNode();

    FMOD_Memory_Free(this);

    return FMOD_OK;
}

FMOD_RESULT ChannelGroupI::release() {
    if (this == this->mSystem->mChannelGroup) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    return this->releaseInternal();
}

FMOD_RESULT ChannelGroupI::validate(ChannelGroup* channelgroup, ChannelGroupI** channelgroupi) {
    if (!channelgroup) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!channelgroupi) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *channelgroupi = reinterpret_cast<ChannelGroupI*>(channelgroup);

    return FMOD_OK;
}

} // namespace FMOD
