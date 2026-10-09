#include "fmod.hpp"
#include "xmodex/channel/ChannelI.hpp"

namespace FMOD {

FMOD_RESULT F_API Channel::stop() {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->stop();
    }
}

FMOD_RESULT F_API Channel::setPaused(bool paused) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setPaused(paused);
    }
}

FMOD_RESULT F_API Channel::setVolume(float volume) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setVolume(volume, false);
    }
}

FMOD_RESULT F_API Channel::getVolume(float* volume) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (volume) {
            *volume = 0.0f;
        }
        return result;
    } else {
        return channeli->getVolume(volume);
    }
}

FMOD_RESULT F_API Channel::setFrequency(float frequency) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setFrequency(frequency);
    }
}

FMOD_RESULT F_API Channel::getFrequency(float* frequency) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (frequency) {
            *frequency = 0.0f;
        }
        return result;
    } else {
        return channeli->getFrequency(frequency);
    }
}

FMOD_RESULT F_API Channel::setSpeakerMix(float frontleft, float frontright, float center, float lfe, float backleft, float backright, float sideleft, float sideright) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setSpeakerMix(frontleft, frontright, center, lfe, backleft, backright, sideleft, sideright, true);
    }
}

FMOD_RESULT F_API Channel::setPriority(int32_t priority) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setPriority(priority);
    }
}

FMOD_RESULT F_API Channel::getPriority(int32_t* priority) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (priority) {
            *priority = 0;
        }
        return result;
    } else {
        return channeli->getPriority(priority);
    }
}

FMOD_RESULT F_API Channel::setPosition(uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setPosition(position, postype);
    }
}

FMOD_RESULT F_API Channel::getPosition(uint32_t* position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (position) {
            *position = 0;
        }
        return result;
    } else {
        return channeli->getPosition(position, postype);
    }
}

FMOD_RESULT F_API Channel::setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setReverbProperties(prop);
    }
}

FMOD_RESULT F_API Channel::getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->getReverbProperties(prop);
    }
}

FMOD_RESULT F_API Channel::setChannelGroup(ChannelGroup* channelgroup) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setChannelGroup((ChannelGroupI*)channelgroup);
    }
}

FMOD_RESULT F_API Channel::setCallback(FMOD_CHANNEL_CALLBACKTYPE type, FMOD_CHANNEL_CALLBACK callback, int32_t command) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setCallback(type, callback, command);
    }
}

FMOD_RESULT F_API Channel::set3DAttributes(const FMOD_VECTOR* pos, const FMOD_VECTOR* vel) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->set3DAttributes(pos, vel);
    }
}

FMOD_RESULT F_API Channel::get3DAttributes(FMOD_VECTOR* pos, FMOD_VECTOR* vel) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (pos) {
            pos->x = pos->y = pos->z = 0.0f;
        }
        if (vel) {
            vel->x = vel->y = vel->z = 0.0f;
        }
        return result;
    } else {
        return channeli->get3DAttributes(pos, vel);
    }
}

FMOD_RESULT F_API Channel::set3DMinMaxDistance(float mindistance, float maxdistance) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->set3DMinMaxDistance(mindistance, maxdistance);
    }
}

FMOD_RESULT F_API Channel::get3DMinMaxDistance(float* mindistance, float* maxdistance) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (mindistance) {
            *mindistance = 0.0f;
        }
        if (maxdistance) {
            *maxdistance = 0.0f;
        }
        return result;
    } else {
        return channeli->get3DMinMaxDistance(mindistance, maxdistance);
    }
}

FMOD_RESULT F_API Channel::set3DConeSettings(float insideconeangle, float outsideconeangle, float outsidevolume) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->set3DConeSettings(insideconeangle, outsideconeangle, outsidevolume);
    }
}

FMOD_RESULT F_API Channel::set3DConeOrientation(FMOD_VECTOR* orientation) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->set3DConeOrientation(orientation);
    }
}

FMOD_RESULT F_API Channel::set3DPanLevel(float level) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->set3DPanLevel(level);
    }
}

FMOD_RESULT F_API Channel::get3DPanLevel(float* level) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (level) {
            *level = 0.0f;
        }
        return result;
    } else {
        return channeli->get3DPanLevel(level);
    }
}

FMOD_RESULT F_API Channel::addDSP(DSP* dsp) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->addDSP(reinterpret_cast<DSPI*>(dsp));
    }
}

FMOD_RESULT F_API Channel::isPlaying(bool* isplaying) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (isplaying) {
            *isplaying = false;
        }
        return result;
    } else {
        return channeli->isPlaying(isplaying);
    }
}

FMOD_RESULT F_API Channel::isVirtual(bool* isvirtual) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (isvirtual) {
            *isvirtual = false;
        }
        return result;
    } else {
        return channeli->isVirtual(isvirtual);
    }
}

FMOD_RESULT F_API Channel::getAudibility(float* audibility) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (audibility) {
            *audibility = 0.0f;
        }
        return result;
    } else {
        return channeli->getAudibility(audibility);
    }
}

FMOD_RESULT F_API Channel::getFadeVolume(float* fadevolume) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (fadevolume) {
            *fadevolume = 0.0f;
        }
        return result;
    } else {
        return channeli->getFadeVolume(fadevolume);
    }
}

FMOD_RESULT F_API Channel::getMode(FMOD_MODE* mode) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (mode) {
            *mode = static_cast<FMOD_MODE>(0);
        }
        return result;
    } else {
        return channeli->getMode(mode);
    }
}

FMOD_RESULT F_API Channel::setUserData(void* _userdata) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channeli->setUserData(_userdata);
    }
}

FMOD_RESULT F_API Channel::getUserData(void** _userdata) {
    FMOD_RESULT result;
    ChannelI* channeli;

    result = ChannelI::validate(this, &channeli);
    if (result != FMOD_OK) {
        if (_userdata) {
            *_userdata = 0;
        }
        return result;
    } else {
        return channeli->getUserData(_userdata);
    }
}

} // namespace FMOD
