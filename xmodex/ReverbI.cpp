#include "xmodex/ReverbI.hpp"
#include "fmod.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/PluginFactory.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

constexpr float kREVERB_GAIN_COEF = 0.002f;
constexpr float k20TH_MAXDISTANCE_GAINDB = -3.0f;
constexpr float kREVERB_GAIN_COEF_INV = 500.0f;

ReverbI::ReverbI() {
    this->mDSP = nullptr;
    this->mChannelData = nullptr;
    this->mGain = 0.0f;
    this->mPresenceGain = nullptr;
    this->mPosition.x = 0.0f;
    this->mPosition.y = 0.0f;
    this->mPosition.z = 0.0f;
    this->mMinRadius = 0.0f;
    this->mMaxRadius = 0.0f;
    this->mSystem = nullptr;
    this->mMinMaxDistance = 0.0f;
    this->m3D = false;
    this->mDisableIfNoEnvironment = false;
    this->mActive = true;
    this->mUserData = nullptr;
}

ReverbI::~ReverbI() {
    this->release(false);
}

bool ReverbI::is3d() {
    return this->m3D;
}

FMOD_RESULT ReverbI::setGain(float lingain) {
    this->mGain = lingain;
    return FMOD_OK;
}

float ReverbI::getGain() {
    return this->mGain;
}

FMOD_REVERB_MODE ReverbI::getMode() {
    return this->mMode;
}

FMOD_RESULT ReverbI::createDSP() {
    if (!this->mSystem) {
        return FMOD_ERR_INITIALIZATION;
    }

    if (!this->mDSP) {
        FMOD_RESULT result;
        int32_t numdsps;

        result = this->mSystem->mPluginFactory->getNumDSPs(&numdsps);
        if (result != FMOD_OK) {
            return result;
        }

        for (int32_t i = 0; i < numdsps; i++) {
            FMOD_DSP_DESCRIPTION_EX* descriptionex = 0;

            result = this->mSystem->mPluginFactory->getDSP(i, &descriptionex);
            if (result != FMOD_OK) {
                continue;
            }

            if (descriptionex->mType == FMOD_DSP_TYPE_SFXREVERB) {
                FMOD_DSP_DESCRIPTION_EX description;

                FMOD_memcpy(&description, descriptionex, sizeof(FMOD_DSP_DESCRIPTION_EX));

                description.channels = this->mSystem->mMaxOutputChannels;

                result = this->mSystem->mPluginFactory->createDSP(&description, &this->mDSP);
                if (result != FMOD_OK) {
                    return result;
                }

                return this->mDSP->setParameter(FMOD_DSP_SFXREVERB_DRYLEVEL, -10000.0f);
            }
        }

        return FMOD_ERR_PLUGIN_MISSING;
    }

    return FMOD_OK;
}

FMOD_RESULT ReverbI::setChanProperties(int32_t index, const FMOD_REVERB_CHANNELPROPERTIES* props, DSPConnection* connection) {
    if (index < 0 || index >= this->mSystem->mNumChannels) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (props) {
        FMOD_memcpy(&this->mChannelData[index], props, sizeof(FMOD_REVERB_CHANNELPROPERTIES));
    }

    if (connection) {
        this->mChannelData[index].mDSPConnection = connection;
    }

    return FMOD_OK;
}

FMOD_RESULT ReverbI::getChanProperties(int32_t index, FMOD_REVERB_CHANNELPROPERTIES* props, DSPConnection** connection) {
    if (index < 0 || index >= this->mSystem->mNumChannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (props) {
        FMOD_memcpy(props, &this->mChannelData[index].mChanProps, sizeof(FMOD_REVERB_CHANNELPROPERTIES));
    }

    if (connection) {
        *connection = this->mChannelData[index].mDSPConnection;
    }

    return FMOD_OK;
}

FMOD_RESULT ReverbI::get3DAttributes(FMOD_VECTOR* position, float* mindistance, float* maxdistance) {
    if (position) {
        *position = this->mPosition;
    }

    if (mindistance) {
        *mindistance = this->mMinRadius;
    }

    if (maxdistance) {
        *maxdistance = this->mMaxRadius;
    }

    return FMOD_OK;
}

FMOD_RESULT ReverbI::setProperties(const FMOD_REVERB_PROPERTIES* prop_source) {
    FMOD_REVERB_PROPERTIES oldprops;
    FMOD_REVERB_PROPERTIES* newprops;

    newprops = &this->mProps;
    FMOD_memcpy(&oldprops, newprops, sizeof(FMOD_REVERB_PROPERTIES));

    newprops->Instance = prop_source->Instance < 0 ? 0 : prop_source->Instance > 3 ? 3
                                                                                   : prop_source->Instance;
    newprops->Environment = prop_source->Environment < -1 ? -1 : prop_source->Environment > 25 ? 25
                                                                                               : prop_source->Environment;
    newprops->EnvSize = prop_source->EnvSize < 1.0f ? 1.0f : prop_source->EnvSize > 100.0f ? 100.0f
                                                                                           : prop_source->EnvSize;
    newprops->EnvDiffusion = prop_source->EnvDiffusion < 0.0f ? 0.0f : prop_source->EnvDiffusion > 1.0f ? 1.0f
                                                                                                        : prop_source->EnvDiffusion;
    newprops->Room = prop_source->Room < -10000 ? -10000 : prop_source->Room > 0 ? 0
                                                                                 : prop_source->Room;
    newprops->RoomHF = prop_source->RoomHF < -10000 ? -10000 : prop_source->RoomHF > 0 ? 0
                                                                                       : prop_source->RoomHF;
    newprops->RoomLF = prop_source->RoomLF < -10000 ? -10000 : prop_source->RoomLF > 0 ? 0
                                                                                       : prop_source->RoomLF;
    newprops->DecayTime = prop_source->DecayTime < 0.1f ? 0.1f : prop_source->DecayTime > 20.0f ? 20.0f
                                                                                                : prop_source->DecayTime;
    newprops->DecayHFRatio = prop_source->DecayHFRatio < 0.1f ? 0.1f : prop_source->DecayHFRatio > 2.0f ? 2.0f
                                                                                                        : prop_source->DecayHFRatio;
    newprops->DecayLFRatio = prop_source->DecayLFRatio < 0.1f ? 0.1f : prop_source->DecayLFRatio > 2.0f ? 2.0f
                                                                                                        : prop_source->DecayLFRatio;
    newprops->Reflections = prop_source->Reflections < -10000 ? -10000 : prop_source->Reflections > 1000 ? 1000
                                                                                                         : prop_source->Reflections;
    newprops->ReflectionsDelay = prop_source->ReflectionsDelay < 0.0f ? 0.0f : prop_source->ReflectionsDelay > 0.3f ? 0.3f
                                                                                                                    : prop_source->ReflectionsDelay;
    newprops->ReflectionsPan[0] = prop_source->ReflectionsPan[0];
    newprops->ReflectionsPan[1] = prop_source->ReflectionsPan[1];
    newprops->ReflectionsPan[2] = prop_source->ReflectionsPan[2];
    newprops->Reverb = prop_source->Reverb < -10000 ? -10000 : prop_source->Reverb > 2000 ? 2000
                                                                                          : prop_source->Reverb;
    newprops->ReverbDelay = prop_source->ReverbDelay < 0.0f ? 0.0f : prop_source->ReverbDelay > 0.1f ? 0.1f
                                                                                                     : prop_source->ReverbDelay;
    newprops->ReverbPan[0] = prop_source->ReverbPan[0];
    newprops->ReverbPan[1] = prop_source->ReverbPan[1];
    newprops->ReverbPan[2] = prop_source->ReverbPan[2];
    newprops->EchoTime = prop_source->EchoTime < 0.075f ? 0.075f : prop_source->EchoTime > 0.25f ? 0.25f
                                                                                                 : prop_source->EchoTime;
    newprops->EchoDepth = prop_source->EchoDepth < 0.0f ? 0.0f : prop_source->EchoDepth > 1.0f ? 1.0f
                                                                                               : prop_source->EchoDepth;
    newprops->ModulationTime = prop_source->ModulationTime < 0.04f ? 0.04f : prop_source->ModulationTime > 4.0f ? 4.0f
                                                                                                                : prop_source->ModulationTime;
    newprops->ModulationDepth = prop_source->ModulationDepth < 0.0f ? 0.0f : prop_source->ModulationDepth > 1.0f ? 1.0f
                                                                                                                 : prop_source->ModulationDepth;
    newprops->AirAbsorptionHF = prop_source->AirAbsorptionHF < -100.0f ? -100.0f : prop_source->AirAbsorptionHF > 0.0f ? 0.0f
                                                                                                                       : prop_source->AirAbsorptionHF;
    newprops->HFReference = prop_source->HFReference < 1000.0f ? 1000.0f : prop_source->HFReference > 20000 ? 20000
                                                                                                            : prop_source->HFReference;
    newprops->LFReference = prop_source->LFReference < 20.0f ? 20.0f : prop_source->LFReference > 1000.0f ? 1000.0f
                                                                                                          : prop_source->LFReference;
    newprops->RoomRolloffFactor = prop_source->RoomRolloffFactor < 0.0f ? 0.0f : prop_source->RoomRolloffFactor > 10.0f ? 10.0f
                                                                                                                        : prop_source->RoomRolloffFactor;
    newprops->Diffusion = prop_source->Diffusion < 0.0f ? 0.0f : prop_source->Diffusion > 100.0f ? 100.0f
                                                                                                 : prop_source->Diffusion;
    newprops->Density = prop_source->Density < 0.0f ? 0.0f : prop_source->Density > 100.0f ? 100.0f
                                                                                           : prop_source->Density;
    newprops->Flags = prop_source->Flags;

    if (this->mDSP) {
        FMOD_RESULT result;
        bool reverb3d_active = false;

        if (this->mSystem) {
            this->mSystem->get3DReverbActive(&reverb3d_active);
        }

        if (this->mDisableIfNoEnvironment) {
            if (newprops->Environment == -1) {
                this->mDSP->setActive(reverb3d_active);
                if (!reverb3d_active) {
                    this->mDSP->reset();
                }
            } else {
                this->mDSP->setActive(true);
            }
        } else {
            this->mDSP->setActive(true);
        }

        if (newprops->Room != oldprops.Room) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_ROOM, static_cast<float>(newprops->Room));
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->RoomHF != oldprops.RoomHF) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_ROOMHF, static_cast<float>(newprops->RoomHF));
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->RoomRolloffFactor != oldprops.RoomRolloffFactor) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_ROOMROLLOFFFACTOR, newprops->RoomRolloffFactor);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->DecayTime != oldprops.DecayTime) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_DECAYTIME, newprops->DecayTime);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->DecayHFRatio != oldprops.DecayHFRatio) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_DECAYHFRATIO, newprops->DecayHFRatio);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->Reflections != oldprops.Reflections) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_REFLECTIONSLEVEL, static_cast<float>(newprops->Reflections));
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->ReflectionsDelay != oldprops.ReflectionsDelay) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_REFLECTIONSDELAY, newprops->ReflectionsDelay);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->Reverb != oldprops.Reverb) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_REVERBLEVEL, static_cast<float>(newprops->Reverb));
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->ReverbDelay != oldprops.ReverbDelay) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_REVERBDELAY, newprops->ReverbDelay);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->Diffusion != oldprops.Diffusion) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_DIFFUSION, newprops->Diffusion);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->Density != oldprops.Density) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_DENSITY, newprops->Density);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->HFReference != oldprops.HFReference) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_HFREFERENCE, newprops->HFReference);
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->RoomLF != oldprops.RoomLF) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_ROOMLF, static_cast<float>(newprops->RoomLF));
            if (result != FMOD_OK) {
                return result;
            }
        }
        if (newprops->LFReference != oldprops.LFReference) {
            result = this->mDSP->setParameter(FMOD_DSP_SFXREVERB_LFREFERENCE, newprops->LFReference);
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    if (this == &this->mSystem->mReverbGlobal) {
        if (this->mSystem->mOutput->mDescription.reverb_setproperties) {
            this->mSystem->mOutput->readfrommixer = Output::mixCallback;
            return this->mSystem->mOutput->mDescription.reverb_setproperties(this->mSystem->mOutput, &this->mProps);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ReverbI::getProperties(FMOD_REVERB_PROPERTIES* properties) {
    if (!properties) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *properties = this->mProps;

    return FMOD_OK;
}

FMOD_RESULT ReverbI::getActive(bool* active) {
    if (!active) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *active = this->mActive;

    return FMOD_OK;
}

FMOD_RESULT ReverbI::setPresenceGain(int32_t index, float lingain) {
    if (index < 0 || index >= this->mSystem->mNumChannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mPresenceGain[index] = lingain;

    return FMOD_OK;
}

FMOD_RESULT ReverbI::getPresenceGain(int32_t index, float* lingain) {
    if (index < 0 || index >= this->mSystem->mNumChannels || !lingain) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *lingain = this->mPresenceGain[index];

    return FMOD_OK;
}

void ReverbI::setDisableIfNoEnvironment(bool dis) {
    FMOD_REVERB_PROPERTIES props;

    this->mDisableIfNoEnvironment = dis;

    this->getProperties(&props);
    this->setProperties(&props);
}

FMOD_RESULT ReverbI::releaseDSP() {
    if (this->mDSP) {
        FMOD_RESULT result;

        result = this->mDSP->disconnectFrom(nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mDSP->release(true);
        if (result != FMOD_OK) {
            return result;
        }
        this->mDSP = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT ReverbI::release(bool freethis) {
    if (this->mPresenceGain) {
        FMOD_Memory_Free(this->mPresenceGain);
        this->mPresenceGain = nullptr;
    }

    if (this->mChannelData) {
        FMOD_Memory_Free(this->mChannelData);
        this->mChannelData = nullptr;
    }

    this->releaseDSP();

    this->removeNode();

    if (this->mSystem && !this->mSystem->count3DPhysicalReverbs()) {
        this->mSystem->mReverb3D.setDisableIfNoEnvironment(true);
    }

    if (this->mSystem && !this->mSystem->count3DVirtualReverbs()) {
        this->mSystem->set3DReverbActive(false);
    }

    if (freethis) {
        FMOD_Memory_Free(this);
    }

    return FMOD_OK;
}

FMOD_RESULT ReverbI::init(SystemI* system, bool is_3d, FMOD_REVERB_MODE mode) {
    FMOD_REVERB_PROPERTIES prop = FMOD_PRESET_OFF;

    this->release(false);

    this->mSystem = system;
    this->mMode = mode;

    FMOD_memset(&this->mProps, 204, sizeof(FMOD_REVERB_PROPERTIES));
    this->setProperties(&prop);

    this->mChannelData = static_cast<FMOD_REVERB_CHANNELDATA*>(FMOD_Memory_Calloc(this->mSystem->mNumChannels * sizeof(FMOD_REVERB_CHANNELDATA)));
    if (!mChannelData) {
        return FMOD_ERR_MEMORY;
    }

    for (int32_t channel = 0; channel < this->mSystem->mNumChannels; channel++) {
        FMOD_memset(&this->mChannelData[channel].mChanProps, 0, sizeof(FMOD_REVERB_CHANNELPROPERTIES));
        this->mChannelData[channel].mChanProps.Room = 0;
        this->mChannelData[channel].mChanProps.Direct = 0;
    }

    this->mPresenceGain = static_cast<float*>(FMOD_Memory_Calloc(this->mSystem->mNumChannels * sizeof(float)));
    if (!this->mPresenceGain) {
        return FMOD_ERR_MEMORY;
    }
    FMOD_memset(this->mPresenceGain, 0, this->mSystem->mNumChannels * sizeof(float));

    this->mPosition.x = this->mPosition.y = this->mPosition.z = 0.0f;
    this->mMinRadius = 0.0f;
    this->mMaxRadius = 0.0f;
    this->mMinMaxDistance = 0.0f;
    this->mGain = 0.0f;
    this->m3D = is_3d;

    if (this == &system->mReverbGlobal) {
        this->mDisableIfNoEnvironment = true;
    }

    if (this == &system->mReverb3D) {
        this->mDisableIfNoEnvironment = true;
    }

    return FMOD_OK;
}

void ReverbI::calculateDistanceGain(FMOD_VECTOR* p, float* linear_gain, float* linear_coeff) {
    FMOD_VECTOR a_vector;
    float a;
    float g;
    float c;

    if (!this->m3D) {
        if (linear_gain) {
            *linear_gain = 1.0f;
        }
        if (linear_coeff) {
            *linear_coeff = 1.0f;
        }
        return;
    }

    FMOD_Vector_Subtract(p, &this->mPosition, &a_vector);

    a = FMOD_Vector_GetLength(&a_vector);

    if (a <= this->mMinRadius) {
        g = c = 1.0f;
    } else if (a >= this->mMaxRadius) {
        g = c = 0.0f;
    } else {
        if (this->mMinMaxDistance <= 0) {
            g = c = 1.0f;
        } else {
            float rolloff = (a - this->mMinRadius) / this->mMinMaxDistance;

            c = 1.0f - rolloff;
            g = FMOD_POW(10.0f, rolloff * k20TH_MAXDISTANCE_GAINDB);
        }
    }

    if (linear_gain) {
        *linear_gain = g;
    }
    if (linear_coeff) {
        *linear_coeff = c;
    }
}

void ReverbI::sumRoomProps(FMOD_REVERB_STDPROPERTIES* accprops, FMOD_REVERB_PROPERTIES* addprops, float factor) {
    if (accprops && addprops) {
        accprops->Room += factor * FMOD_EXP(static_cast<float>(addprops->Room) * kREVERB_GAIN_COEF);
    }
}

void ReverbI::sumProps(FMOD_REVERB_STDPROPERTIES* accprops, FMOD_REVERB_PROPERTIES* addprops, float factor) {
    if (accprops && addprops) {
        accprops->Room += factor * FMOD_EXP(static_cast<float>(addprops->Room) * kREVERB_GAIN_COEF);
        accprops->RoomHF += factor * FMOD_EXP(static_cast<float>(addprops->RoomHF) * kREVERB_GAIN_COEF);
        accprops->RoomLF += factor * FMOD_EXP(static_cast<float>(addprops->RoomLF) * kREVERB_GAIN_COEF);
        accprops->RoomRolloffFactor += factor * addprops->RoomRolloffFactor;
        accprops->DecayTime += factor * addprops->DecayTime;
        accprops->DecayHFRatio += factor * addprops->DecayHFRatio;
        accprops->Reflections += factor * FMOD_EXP(static_cast<float>(addprops->Reflections) * kREVERB_GAIN_COEF);
        accprops->ReflectionsDelay += factor * addprops->ReflectionsDelay;
        accprops->Reverb += factor * FMOD_EXP((float)(addprops->Reverb * kREVERB_GAIN_COEF));
        accprops->ReverbDelay += factor * addprops->ReverbDelay;
        accprops->Diffusion += factor * addprops->Diffusion;
        accprops->Density += factor * addprops->Density;
        accprops->HFReference += (addprops->HFReference > 0.0f) ? factor * FMOD_LOG(addprops->HFReference) : 0.0f;
        accprops->LFReference += (addprops->LFReference > 0.0f) ? factor * FMOD_LOG(addprops->LFReference) : 0.0f;
    }
}

void ReverbI::factorProps(FMOD_REVERB_PROPERTIES* quotientprops, FMOD_REVERB_STDPROPERTIES* divprops, float factor) {
    FMOD_memset(quotientprops, 0, sizeof(FMOD_REVERB_PROPERTIES));

    if (quotientprops && divprops) {
        quotientprops->Room = (divprops->Room > 0.0f) ? static_cast<int32_t>(0.5f + FMOD_LOG(divprops->Room) * kREVERB_GAIN_COEF_INV * factor) : -10000;
        quotientprops->Room = (divprops->Room > 0.0f) ? static_cast<int32_t>(0.5f + FMOD_LOG(divprops->Room * factor) * kREVERB_GAIN_COEF_INV) : -10000;
        quotientprops->RoomHF = (divprops->RoomHF > 0.0f) ? static_cast<int32_t>(0.5f + FMOD_LOG(divprops->RoomHF * factor) * kREVERB_GAIN_COEF_INV) : -10000;
        quotientprops->RoomLF = (divprops->RoomLF > 0.0f) ? static_cast<int32_t>(0.5f + FMOD_LOG(divprops->RoomLF * factor) * kREVERB_GAIN_COEF_INV) : -10000;
        quotientprops->RoomRolloffFactor = divprops->RoomRolloffFactor * factor;
        quotientprops->DecayTime = divprops->DecayTime * factor;
        quotientprops->DecayHFRatio = divprops->DecayHFRatio * factor;
        quotientprops->Reflections = (divprops->Reflections > 0.0f) ? static_cast<int32_t>(0.5f + FMOD_LOG(divprops->Reflections * factor) * kREVERB_GAIN_COEF_INV) : -10000;
        quotientprops->ReflectionsDelay = divprops->ReflectionsDelay * factor;
        quotientprops->Reverb = (divprops->Reverb > 0.0f) ? static_cast<int32_t>(0.5f + FMOD_LOG(divprops->Reverb * factor) * kREVERB_GAIN_COEF_INV) : -10000;
        quotientprops->ReverbDelay = divprops->ReverbDelay * factor;
        quotientprops->Diffusion = divprops->Diffusion * factor;
        quotientprops->Density = divprops->Density * factor;
        quotientprops->HFReference = FMOD_EXP(divprops->HFReference * factor);
        quotientprops->LFReference = FMOD_EXP(divprops->LFReference * factor);
    }
}

} // namespace FMOD
