#include "xmodex/channel/ChannelI.hpp"
#include "fmod.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/SpeakerMap.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SyncPoint.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/Types.hpp"
#include "xmodex/channel/ChannelReal.hpp"
#include "xmodex/channel/ChannelStream.hpp"
#include "xmodex/codec/Codec.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/output/OutputEmulated.hpp"
#include "xmodex/sound/Sample.hpp"
#include "xmodex/sound/SoundGroupI.hpp"

namespace FMOD {

ChannelI::ChannelI() {
    this->init();
}

ChannelI::ChannelI(int32_t index, SystemI* system) {
    this->init();

    this->mSystem = system;
    this->mIndex = index;

    this->mHandleCurrent = ((index & 0xFFF | (system->mIndex << 12)) << 16) | 1;
    this->mHandleOriginal = this->mHandleCurrent;
}

FMOD_RESULT ChannelI::addDSP(DSPI* dsp) {
    FMOD_RESULT result;
    DSPI* dsphead;
    DSPI* inputdsp = nullptr;
    int32_t numinputs;

    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    result = this->mRealChannel[0]->getDSPHead(&dsphead);
    if (result != FMOD_OK) {
        return result;
    }

    result = dsphead->getNumInputs(&numinputs);
    if (result != FMOD_OK) {
        return result;
    }

    if (numinputs > 1) {
        return FMOD_ERR_DSP_TOOMANYCONNECTIONS;
    }

    if (dsp->mFlags & FMOD_DSP_FLAG_USEDADDDSP) {
        result = dsp->remove();
        if (result != FMOD_OK) {
            return result;
        }
    } else {
        result = dsp->disconnectFrom(nullptr);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = dsp->getNumInputs(&numinputs);
    if (result != FMOD_OK) {
        return result;
    }

    if (numinputs > 0) {
        return FMOD_ERR_DSP_CONNECTION;
    }

    result = dsphead->getInput(0, &inputdsp);
    if (result == FMOD_OK) {
        result = dsphead->disconnectFrom(inputdsp);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = dsphead->addInput(dsp);
    if (result != FMOD_OK) {
        return result;
    }

    if (inputdsp) {
        result = dsp->addInput(inputdsp);
        if (result != FMOD_OK) {
            return result;
        }
    }

    dsp->reset();
    dsp->mFlags |= (FMOD_DSP_FLAG_ACTIVE | FMOD_DSP_FLAG_USEDADDDSP);
    this->mFlags |= CHANNELI_FLAG_USEDADDDSP;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::addDSPChain(DSPI* dsp) {
    FMOD_RESULT result;
    DSPI* dsphead;
    DSPI* inputdsp = nullptr;
    int32_t numinputs;

    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    result = this->mRealChannel[0]->getDSPHead(&dsphead);
    if (result != FMOD_OK) {
        return result;
    }

    result = dsphead->getNumInputs(&numinputs);
    if (result != FMOD_OK) {
        return result;
    }

    if (numinputs > 1) {
        return FMOD_ERR_DSP_TOOMANYCONNECTIONS;
    }

    if (!(dsp->mFlags & FMOD_DSP_FLAG_USEDADDDSP)) {
        return FMOD_ERR_DSP_CONNECTION;
    }

    result = dsphead->getInput(0, &inputdsp);
    if (result == FMOD_OK) {
        result = dsphead->disconnectFrom(inputdsp);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = dsphead->addInput(dsp);
    if (result != FMOD_OK) {
        return result;
    }

    if (inputdsp) {
        auto current = dsp;
        auto root = dsp;
        while (current->getInput(0, &current) == FMOD_OK) {
            if (!(current->mFlags & FMOD_DSP_FLAG_USEDADDDSP)) {
                break;
            }
            root = current;
        }

        result = root->addInput(inputdsp);
        if (result != FMOD_OK) {
            return result;
        }
    }

    dsp->reset();
    dsp->mFlags |= (FMOD_DSP_FLAG_ACTIVE | FMOD_DSP_FLAG_USEDADDDSP);
    this->mFlags |= CHANNELI_FLAG_USEDADDDSP;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::alloc(DSPI* dsp, bool reset) {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        this->mRealChannel[i]->mSubChannelIndex = i;
        this->mRealChannel[i]->mSound = nullptr;
        this->mRealChannel[i]->mDSP = dsp;
        this->mRealChannel[i]->mLoopStart = 0;
        this->mRealChannel[i]->mLoopLength = static_cast<uint32_t>(-1);
        this->mRealChannel[i]->mLoopCount = -1;
        this->mRealChannel[i]->mMode = FMOD_2D | FMOD_SOFTWARE;
        this->mRealChannel[i]->mDelayStart = 0;
        this->mRealChannel[i]->mDelayEnd = 0;
        this->mRealChannel[i]->mParent = this;
    }

    if (reset) {
        this->mFlags &= ~(CHANNELI_FLAG_REALMUTE | CHANNELI_FLAG_PAUSED | CHANNELI_FLAG_MUTED | CHANNELI_FLAG_FORCEVIRTUAL);

        for (int32_t i = 0; i < FMOD_CHANNEL_MAXINPUTCHANNELS; i++) {
            this->mInputMix[i] = 1.0f;
        }

        this->mVolume3D = 1.0f;
        this->mConeVolume3D = 1.0f;
        this->mFlags &= ~CHANNELI_FLAG_MOVED;
        this->mPitch3D = 1.0f;
        this->mRolloffPoint = nullptr;
        this->mDirectOcclusionVolume = 1.0f;
        this->mNumRolloffPoints = 0;
        this->mReverbDryVolume = 1.0f;
        this->mMinDistance = 1.0f;
        this->mMaxDistance = 10000.0f;
        this->mDistance = 0.0f;
        this->mConeInsideAngle = 360.0f;
        this->mConeOutsideAngle = 360.0f;
        this->mConeOutsideVolume = 1.0f;
        this->mConeOrientation.z = 1.0f;
        this->mConeOrientation.x = 0.0f;
        this->mConeOrientation.y = 0.0f;
        this->m3DPanLevel = 1.0f;
        this->m3DDopplerLevel = 1.0f;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        result = this->mRealChannel[i]->alloc(dsp);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::alloc(SoundI* sound, bool reset) {
    FMOD_RESULT result;

    if (reset) {
        this->mSyncPointCurrent = nullptr;

        if (this->mSystem) {
            this->mChannelGroup = this->mSystem->mChannelGroup;
        }

        this->mFlags &= ~(CHANNELI_FLAG_REALMUTE | CHANNELI_FLAG_PAUSED | CHANNELI_FLAG_MUTED | CHANNELI_FLAG_FORCEVIRTUAL);

        for (int32_t i = 0; i < FMOD_CHANNEL_MAXINPUTCHANNELS; i++) {
            this->mInputMix[i] = 1.0f;
        }

        this->mVolume3D = 1.0f;
        this->mFlags &= ~CHANNELI_FLAG_MOVED;
        this->mConeVolume3D = 1.0f;
        this->mPitch3D = 1.0f;
        this->mDirectOcclusionVolume = 1.0f;
        this->mReverbDryVolume = 1.0f;
        this->mMinDistance = sound->mMinDistance;
        this->mMaxDistance = sound->mMaxDistance;
        this->mDistance = 0.0f;
        this->mConeInsideAngle = sound->mConeInsideAngle;
        this->mConeOutsideAngle = sound->mConeOutsideAngle;
        this->mConeOutsideVolume = sound->mConeOutsideVolume;
        this->mConeOrientation.x = 0.0f;
        this->mConeOrientation.y = 0.0f;
        this->mConeOrientation.z = 1.0f;
        this->mRolloffPoint = sound->mRolloffPoint;
        this->m3DPanLevel = 1.0f;
        this->m3DDopplerLevel = 1.0f;
        this->mNumRolloffPoints = sound->mNumRolloffPoints;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        if (!this->mRealChannel[i]) {
            return FMOD_ERR_INVALID_HANDLE;
        }

        if (sound->isStream()) {
            this->mRealChannel[i]->mSound = sound;
        } else {
            auto sample = reinterpret_cast<Sample*>(sound);
            this->mRealChannel[i]->mSound = this->mNumRealChannels > 1 ? sample->mSubSample[i] : sound;
        }

        this->mRealChannel[i]->mSubChannelIndex = i;
        this->mRealChannel[i]->mDSP = nullptr;
        this->mRealChannel[i]->mMode = sound->mMode;
        this->mRealChannel[i]->mLoopStart = sound->mLoopStart;
        this->mRealChannel[i]->mLoopLength = sound->mLoopLength;
        this->mRealChannel[i]->mLoopCount = sound->mLoopCount;
        this->mRealChannel[i]->mLength = sound->mLength;
        this->mRealChannel[i]->mDelayStart = 0;
        this->mRealChannel[i]->mDelayEnd = 0;
        this->mRealChannel[i]->mParent = this;

        result = this->mRealChannel[i]->alloc();
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::calcVolumeAndPitchFor3D(int32_t delta) {
    constexpr float SPEED_OF_SOUND = 340.0f;
    FMOD_RESULT result;
    FMOD_VECTOR currdiff;
    FMOD_VECTOR lastdiff;
    FMOD_VECTOR lastpos;
    FMOD_VECTOR tmp;
    Listener* listener;
    float angle;
    float conevol = 1.0f;
    float distance;
    float distancescale;
    float dopplerscale;
    float lastdistance;
    float pitch3d = 1.0f;
    float rolloffscale;
    float smallestdistance;
    float vol3d = 1.0f;
    int32_t numlisteners;

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        if (!(this->mRealChannel[i]->mMode & FMOD_2D)) {

            result = this->mSystem->get3DNumListeners(&numlisteners);
            if (result != FMOD_OK) {
                return result;
            }

            // OFFSET: windows:x86:008DCFD9
            smallestdistance = 999999999.0f;

            for (int32_t j = 0; j < numlisteners; j++) {
                result = this->mSystem->getListenerObject(j, &listener);
                if (result != FMOD_OK) {
                    return result;
                }

                if (!(this->mFlags & CHANNELI_FLAG_MOVED) && !listener->mMoved) {
                    return FMOD_OK;
                }

                result = this->mSystem->get3DSettings(&dopplerscale, &distancescale, &rolloffscale);
                if (result != FMOD_OK) {
                    return result;
                }

                dopplerscale *= this->m3DDopplerLevel;

                if (this->mRealChannel[i]->mMode & FMOD_3D_HEADRELATIVE) {
                    currdiff = this->mPosition3D;
                } else {
                    FMOD_Vector_Subtract(&this->mPosition3D, &listener->mPosition, &currdiff);
                }

                if (this->mSystem->mFlags & FMOD_INIT_3D_RIGHTHANDED) {
                    currdiff.z = -currdiff.z;
                }

                distance = FMOD_Vector_GetLength(&currdiff);

                if (distance < smallestdistance) {
                    smallestdistance = distance;
                    this->mDistance = distance;

                    if (this->mSystem->mRolloffCallback) {
                        vol3d = this->mSystem->mRolloffCallback(
                            reinterpret_cast<FMOD_CHANNEL*>(this->mRealChannel[i]->mParent->mHandleCurrent),
                            this->mDistance);
                    } else if (this->mRealChannel[i]->mMode & FMOD_3D_CUSTOMROLLOFF) {
                        if (this->mRolloffPoint && this->mNumRolloffPoints) {
                            if (distance >= this->mRolloffPoint[this->mNumRolloffPoints - 1].x) {
                                vol3d = this->mRolloffPoint[this->mNumRolloffPoints - 1].y;
                            } else {
                                for (int32_t k = 1; k < this->mNumRolloffPoints; k++) {
                                    if (distance >= this->mRolloffPoint[k - 1].x && distance < this->mRolloffPoint[k].x) {
                                        float frac = (distance - this->mRolloffPoint[k - 1].x) / (this->mRolloffPoint[k].x - this->mRolloffPoint[k - 1].x);

                                        vol3d = ((1.0f - frac) * this->mRolloffPoint[k - 1].y) + (frac * this->mRolloffPoint[k].y);
                                        break;
                                    }
                                }
                            }
                        } else {
                            vol3d = 1.0f;
                        }
                    } else {
                        if (distance >= this->mMaxDistance) {
                            distance = this->mMaxDistance;
                        }
                        if (distance < this->mMinDistance) {
                            distance = this->mMinDistance;
                        }

                        if (this->mRealChannel[i]->mMode & FMOD_3D_LINEARROLLOFF) {
                            float range;

                            range = this->mMaxDistance - this->mMinDistance;
                            if (range <= 0) {
                                vol3d = 1.0f;
                            } else {
                                distance = this->mMaxDistance - distance;
                                vol3d = distance / range;
                            }
                        } else {
                            if ((distance > this->mMinDistance) && (rolloffscale != 1.0f)) {
                                distance -= this->mMinDistance;
                                distance *= rolloffscale;
                                distance += this->mMinDistance;
                            }

                            if (distance < 0.000001f) {
                                distance = 0.000001f;
                            }

                            vol3d = this->mMinDistance / distance;
                        }
                    }

                    if (vol3d < 0.0f) {
                        vol3d = 0.0f;
                    }
                    if (vol3d > 1.0f) {
                        vol3d = 1.0f;
                    }

                    if (this->mConeOutsideAngle < 360.0f || this->mConeInsideAngle < 360.0f) {
                        if (this->mDistance <= 0) {
                            angle = 0;
                        } else {
                            float scale = 1.0f / this->mDistance;
                            FMOD_VECTOR coneorientation = this->mConeOrientation;

                            if (this->mSystem->mFlags & FMOD_INIT_3D_RIGHTHANDED) {
                                coneorientation.z = -coneorientation.z;
                            }

                            FMOD_Vector_Scale(&currdiff, scale, &currdiff);

                            angle = -FMOD_Vector_DotProduct(&currdiff, &coneorientation);
                            angle = angle < -1.0f ? -1.0f : angle > 1.0f ? 1.0f
                                                                         : angle;
                            angle = FMOD_ACOS(angle) / FMOD_PI * 180.0f;
                            angle *= 2;
                        }

                        if (angle < this->mConeInsideAngle) {
                            conevol = 1.0f;
                        } else if (angle < this->mConeOutsideAngle) {
                            float f = (angle - this->mConeInsideAngle) / (this->mConeOutsideAngle - this->mConeInsideAngle);
                            conevol = (this->mConeOutsideVolume * f) + (1.0f * (1.0f - f));
                        } else {
                            conevol = this->mConeOutsideVolume;
                        }
                    }
                }

                if ((dopplerscale > 0) && (numlisteners == 1)) {
                    float delta_sec = (delta <= 0 || delta > 1000) ? 1.0f : delta / 1000.0f;

                    FMOD_Vector_Scale(&this->mVelocity3D, delta_sec, &tmp);
                    FMOD_Vector_Subtract(&this->mPosition3D, &tmp, &lastpos);

                    if (this->mRealChannel[i]->mMode & FMOD_3D_HEADRELATIVE) {
                        lastdiff = lastpos;
                    } else {
                        FMOD_VECTOR listenerlastpos;

                        FMOD_Vector_Scale(&listener->mVelocity, delta_sec, &tmp);
                        FMOD_Vector_Subtract(
                            &listener->mPosition,
                            &tmp,
                            &listenerlastpos);

                        FMOD_Vector_Subtract(&lastpos, &listenerlastpos, &lastdiff);
                    }

                    lastdistance = FMOD_Vector_GetLength(&lastdiff);

                    pitch3d = SPEED_OF_SOUND * distancescale;
                    pitch3d -= ((this->mDistance - lastdistance) / delta_sec * dopplerscale);
                    pitch3d /= (SPEED_OF_SOUND * distancescale);
                }

                if (pitch3d < 0.000001f) {
                    pitch3d = 0.000001f;
                }
            }
        }
    }

    this->mVolume3D = vol3d;
    this->mConeVolume3D = conevol;
    this->mPitch3D = pitch3d;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::calculate3DReverbGain(ReverbI* reverb, FMOD_VECTOR* channelpos, float* gain) {
    if (!gain) {
        return FMOD_OK;
    }

    if (reverb == &this->mSystem->mReverb3D) {
        *gain = 1.0f;
    } else {
        reverb->calculateDistanceGain(channelpos, gain, nullptr);
    }

    if (*gain > 0.0f) {
        FMOD_VECTOR reverbpos;
        float direct_o;
        float reverb_o;

        reverb->get3DAttributes(&reverbpos, nullptr, nullptr);

        // WORKAROUND:
        // since the client never uses geometry data,
        // and the GeometryMgr is not yet implemented
        // initialize reverb occlusion to 0.0f
        // TODO
        // this->mSystem->mGeometryMgr.lineTestAll(channelpos, &reverbpos, &direct_o, &reverb_o);
        reverb_o = 0.0f;

        *gain *= (1.0f - reverb_o);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::get3DAttributes(FMOD_VECTOR* pos, FMOD_VECTOR* vel) {
    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (pos) {
        pos->x = this->mPosition3D.x;
        pos->y = this->mPosition3D.y;
        pos->z = this->mPosition3D.z;
    }

    if (vel) {
        vel->x = this->mVelocity3D.x;
        vel->y = this->mVelocity3D.y;
        vel->z = this->mVelocity3D.z;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::get3DMinMaxDistance(float* mindistance, float* maxdistance) {
    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (mindistance) {
        *mindistance = this->mMinDistance;
    }

    if (maxdistance) {
        *maxdistance = this->mMaxDistance;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::get3DPanLevel(float* level) {
    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (!level) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *level = this->m3DPanLevel;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getAudibility(float* audibility) {
    if (!audibility) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mRealChannel[0]->mMode & FMOD_3D) {
        *audibility = this->mChannelGroup->mRealDirectOcclusionVolume *
                      this->mChannelGroup->mRealVolume *
                      this->mConeVolume3D *
                      this->mVolume3D *
                      this->mReverbDryVolume *
                      this->mDirectOcclusionVolume *
                      this->mVolume;
        return FMOD_OK;
    }

    *audibility = this->mChannelGroup->mRealVolume * this->mVolume;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getChannelInfo(FMOD_CHANNEL_INFO* info) {
    FMOD_RESULT result;

    info->mRealChannel = this->mRealChannel[0];
    info->mAddDSP = nullptr;
    info->mLevels = this->mLevels;
    info->mSound = nullptr;
    info->mDSP = nullptr;

    this->getMode(&info->mMode);
    this->getPosition(&info->mPCM, FMOD_TIMEUNIT_PCM);

    this->getLoopPoints(&info->mLoopStart, FMOD_TIMEUNIT_PCM, &info->mLoopEnd, FMOD_TIMEUNIT_PCM);

    this->getCurrentSound(&info->mSound);
    if (!info->mSound) {
        this->getCurrentDSP(&info->mDSP);
    }

    this->getLoopCount(&info->mLoopCount);
    this->getMute(&info->mMute);
    this->getPaused(&info->mPaused);

    this->getDelay(&info->mDelayStart, &info->mDelayEnd);

    this->getReverbProperties(&info->mReverbProperties);

    DSPI* dsphead = nullptr;
    if (this->mRealChannel[0]) {
        this->mRealChannel[0]->getDSPHead(&dsphead);
    }

    if (dsphead && (this->mFlags & CHANNELI_FLAG_USEDADDDSP)) {
        result = dsphead->getInput(0, &info->mAddDSP);
        if (result == FMOD_OK) {
            if (info->mAddDSP && !(info->mAddDSP->mFlags & FMOD_DSP_FLAG_USEDADDDSP)) {
                info->mAddDSP = nullptr;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getCurrentDSP(DSPI** dsp) {
    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        *dsp = nullptr;
        return FMOD_ERR_INVALID_HANDLE;
    }

    *dsp = this->mRealChannel[0]->mDSP;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getCurrentSound(SoundI** sound) {
    if (!sound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        *sound = nullptr;
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mRealChannel[0]->mSound) {
        *sound = this->mRealChannel[0]->mSound->mSubSampleParent;
        return FMOD_OK;
    }

    *sound = nullptr;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getDelay(uint32_t* startdelay, uint32_t* enddelay) {
    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (startdelay) {
        *startdelay = this->mRealChannel[0]->mDelayStart;
    }

    if (startdelay) {
        *startdelay = this->mRealChannel[0]->mDelayEnd;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getFadeVolume(float* fadevolume) {
    if (!fadevolume) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *fadevolume = this->mFadeVolume;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getFrequency(float* frequency) {
    if (!frequency) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *frequency = this->mFrequency;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getLoopCount(int32_t* loopcount) {
    if (!loopcount) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *loopcount = this->mRealChannel[0]->mLoopCount;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getLoopPoints(uint32_t* loopstart, FMOD_TIMEUNIT loopstarttype, uint32_t* loopend, FMOD_TIMEUNIT loopendtype) {
    SoundI* soundi;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if ((loopstarttype != FMOD_TIMEUNIT_MS &&
         loopstarttype != FMOD_TIMEUNIT_PCM &&
         loopstarttype != FMOD_TIMEUNIT_PCMBYTES) ||
        (loopendtype != FMOD_TIMEUNIT_MS &&
         loopendtype != FMOD_TIMEUNIT_PCM &&
         loopendtype != FMOD_TIMEUNIT_PCMBYTES)) {
        return FMOD_ERR_FORMAT;
    }

    soundi = static_cast<SoundI*>(this->mRealChannel[0]->mSound);
    if (!soundi) {
        return FMOD_ERR_INVALID_PARAM;
    }

    soundi = soundi->mSubSampleParent;

    if (loopstart) {
        if (loopstarttype == FMOD_TIMEUNIT_PCM) {
            *loopstart = this->mRealChannel[0]->mLoopStart;
        } else if (loopstarttype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getBytesFromSamples(this->mRealChannel[0]->mLoopStart, loopstart, soundi->mChannels, soundi->mFormat);
        } else if (loopstarttype == FMOD_TIMEUNIT_MS) {
            *loopstart = static_cast<uint32_t>(static_cast<float>(this->mRealChannel[0]->mLoopStart) * 1000.0f / soundi->mDefaultFrequency);
        }
    }

    if (loopend) {
        uint32_t lend = this->mRealChannel[0]->mLoopStart + this->mRealChannel[0]->mLoopLength - 1;

        if (loopendtype == FMOD_TIMEUNIT_PCM) {
            *loopend = lend;
        } else if (loopendtype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getBytesFromSamples(lend, loopend, soundi->mChannels, soundi->mFormat);
        } else if (loopendtype == FMOD_TIMEUNIT_MS) {
            *loopend = static_cast<uint32_t>(static_cast<float>(lend) * 1000.0f / soundi->mDefaultFrequency);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getMode(FMOD_MODE* mode) {
    if (!mode) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *mode = this->mRealChannel[0]->mMode;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getMute(bool* mute) {
    if (!mute) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mFlags & CHANNELI_FLAG_MUTED) {
        *mute = true;
    } else {
        *mute = false;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getPaused(bool* paused) {
    if (!paused) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *paused = this->mFlags & CHANNELI_FLAG_PAUSED ? true : false;
    return FMOD_OK;
}

FMOD_RESULT ChannelI::getPosition(uint32_t* position, FMOD_TIMEUNIT postype) {
    if (!position) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    return this->mRealChannel[0]->getPosition(position, postype);
}

FMOD_RESULT ChannelI::getPriority(int32_t* priority) {
    if (!priority) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *priority = this->mPriority;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getRealChannel(ChannelReal** realchan, int32_t* subchannels) {
    if (this->mRealChannel[0]->isStream()) {
        auto channelstream = static_cast<ChannelStream*>(this->mRealChannel[0]);

        if (realchan) {
            for (int32_t i = 0; i < channelstream->mNumSubChannels; i++) {
                realchan[i] = channelstream->mSubChannel[i];
            }
        }

        if (subchannels) {
            *subchannels = channelstream->mNumSubChannels;
        }
    } else {
        if (realchan) {
            for (int32_t i = 0; i < this->mNumRealChannels; i++) {
                realchan[i] = this->mRealChannel[i];
            }
        }

        if (subchannels) {
            *subchannels = this->mNumRealChannels;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        auto result2 = this->mRealChannel[i]->getReverbProperties(prop);
        if (result == FMOD_OK) {
            result = result2;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::getSpeakerLevels(FMOD_SPEAKER speaker, float* levels, int32_t numlevels) {
    if (!levels || !numlevels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (speaker >= FMOD_SPEAKER_MAX) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mLevels) {
        for (int32_t i = 0; i < numlevels; i++) {
            levels[i] = this->mLevels[(this->mSystem->mMaxInputChannels * speaker) + i];
        }
    } else {
        for (int32_t i = 0; i < numlevels; i++) {
            levels[i] = 0.0f;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::getVolume(float* volume) {
    if (!volume) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *volume = this->mVolume;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::init() {
    this->mSystem = nullptr;
    this->mHandleCurrent = 0;
    this->mHandleOriginal = 0;

    for (int32_t i = 0; i < FMOD_CHANNEL_CALLBACKTYPE_MAX; i++) {
        this->mCallback[0] = nullptr;
        this->mCallbackCommand[0] = 0;
    }

    for (int32_t i = 0; i < FMOD_CHANNEL_MAXREALSUBCHANNELS; i++) {
        this->mRealChannel[i] = nullptr;
    }

    for (int32_t i = 0; i < FMOD_CHANNEL_MAXINPUTCHANNELS; i++) {
        this->mInputMix[i] = 1.0f;
    }

    this->mVolume = 1.0f;
    this->mIndex = 0;
    this->mFadeVolume = 1.0f;
    this->mLevels = nullptr;
    this->mFadeTarget = 1.0f;
    this->mRolloffPoint = nullptr;
    this->mNumRolloffPoints = 0;
    this->mFrequency = 44100.0f;
    this->mFlags = 0;
    this->mNumRealChannels = 1;
    this->mPan = 0.0f;
    this->mPriority = FMOD_CHANNEL_DEFAULTPRIORITY;
    this->mListPosition = static_cast<uint32_t>(-1);
    this->mSpeakerFL = 1.0f;
    this->mSpeakerFR = 1.0f;
    this->mSpeakerC = 1.0f;
    this->mSpeakerLFE = 1.0f;
    this->mSpeakerBL = 1.0f;
    this->mSpeakerBR = 1.0f;
    this->mSpeakerSL = 1.0f;
    this->mSpeakerSR = 1.0f;
    this->mVolume3D = 1.0f;
    this->mPitch3D = 1.0f;
    this->mConeVolume3D = 1.0f;
    this->mDirectOcclusionVolume = 1.0f;
    this->mReverbDryVolume = 1.0f;
    this->mMinDistance = 1.0f;
    this->mPosition3D.x = 0.0;
    this->mPosition3D.y = 0.0;
    this->mPosition3D.z = 0.0;
    this->mVelocity3D.x = 0.0;
    this->mVelocity3D.y = 0.0;
    this->mVelocity3D.z = 0.0;
    this->mMaxDistance = 1000000000.0f;

    this->mChannelGroupNode.initNode();

    return FMOD_OK;
}

FMOD_RESULT ChannelI::isPlaying(bool* isplaying) {
    FMOD_RESULT result;
    if (!isplaying) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *isplaying = false;

    if (this->mHandleCurrent != this->mHandleOriginal) {
        return FMOD_ERR_CHANNEL_STOLEN;
    }

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_STOPPED) {
        *isplaying = false;
        return FMOD_OK;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        bool isplayingtemp;

        result = this->mRealChannel[i]->isPlaying(&isplayingtemp, false);
        if (result != FMOD_OK) {
            return result;
        }

        if (isplayingtemp) {
            *isplaying = true;
            return FMOD_OK;
        }
    }

    if (!*isplaying) {
        this->mListPosition = static_cast<uint32_t>(-1);

        if (this->mSortedListNode.getData()) {
            this->mSortedListNode.removeNode();
            this->mSortedListNode.addBefore(&this->mSystem->mChannelSortedListHead);
            this->mSortedListNode.setData(this);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::isVirtual(bool* isvirtual) {
    if (!isvirtual) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (!this->mRealChannel[0]) {
        *isvirtual = false;
        return FMOD_ERR_INVALID_HANDLE;
    }

    return this->mRealChannel[0]->isVirtual(isvirtual);
}

FMOD_RESULT ChannelI::play(DSPI* dsp, bool paused, bool reset, bool startmuted) {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    result = this->alloc(dsp, reset);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->setPaused(true);
    if (result != FMOD_OK) {
        return result;
    }

    if (startmuted) {
        this->mFadeVolume = 0.0f;
        this->mFadeTarget = 0.0f;
    } else {
        this->mFadeVolume = 1.0f;
        this->mFadeTarget = 1.0f;
    }

    if (reset) {
        result = this->setDefaults();
        if (result != FMOD_OK) {
            return result;
        }

        result = this->setPosition(0, FMOD_TIMEUNIT_PCM);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->start();
    if (result != FMOD_OK) {
        return result;
    }

    if (reset) {
        FMOD_MODE mode;
        this->getMode(&mode);

        if (mode & FMOD_3D) {
            FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

            result = this->set3DAttributes(
                &this->mSystem->mListener[0].mPosition,
                &vel);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->update(0);
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    if (!paused) {
        result = this->setPaused(false);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::play(SoundI* sound, bool paused, bool reset, bool startmuted) {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    result = this->alloc(sound, reset);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->setPaused(true);
    if (result != FMOD_OK) {
        return result;
    }

    if (startmuted) {
        this->mFadeVolume = 0.0f;
        this->mFadeTarget = 0.0f;
    } else {
        this->mFadeVolume = 1.0f;
        this->mFadeTarget = 1.0f;
    }

    if (reset) {
        result = this->setDefaults();
        if (result != FMOD_OK) {
            return result;
        }

        result = this->setPosition(0, FMOD_TIMEUNIT_PCM);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->start();
    if (result != FMOD_OK) {
        return result;
    }

    sound->mNumAudible++;

    if (sound->mSoundGroup && sound->mSystem) {
        sound->mSoundGroup->removeNode();
        sound->mSoundGroup->addAfter(&this->mSystem->mSoundGroupUsedHead);
    }

    if (reset) {
        FMOD_MODE mode;
        sound->getMode(&mode);

        if (mode & FMOD_3D) {
            FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };

            result = this->set3DAttributes(
                &this->mSystem->mListener[0].mPosition,
                &vel);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->update(0);
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    if (sound->mSyncPointHead && sound->mNumSyncPoints) {
        this->mSyncPointCurrent = sound->mSyncPointHead;
    }

    if (!paused) {
        result = this->setPaused(false);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::referenceStamp(bool newstamp) {
    uint32_t sysindex;
    uint32_t index;
    uint32_t refcount;

    sysindex = this->mHandleCurrent >> 28;
    index = (this->mHandleCurrent >> 16) & 0xFFF;

    refcount = (newstamp ? this->mHandleCurrent : this->mHandleOriginal);

    if (refcount > 0xFFFF) {
        refcount = 1;
    }

    this->mHandleCurrent = refcount | (((sysindex << 12) | index & 0xFFF) << 16);

    if (newstamp) {
        this->mHandleOriginal = this->mHandleCurrent;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::returnToFreeList() {
    if (!this->mSystem) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mSortedListNode.removeNode();

    this->removeNode();

    this->addAfter(&this->mSystem->mChannelFreeListHead);

    return FMOD_OK;
}

FMOD_RESULT ChannelI::set3DAttributes(const FMOD_VECTOR* pos, const FMOD_VECTOR* vel) {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (pos) {
        if (this->mPosition3D.x != pos->x ||
            this->mPosition3D.y != pos->y ||
            this->mPosition3D.z != pos->z) {
            this->mFlags |= CHANNELI_FLAG_MOVED;
        }

        this->mPosition3D = *pos;
    }

    if (vel) {
        if (this->mVelocity3D.x != vel->x ||
            this->mVelocity3D.y != vel->y ||
            this->mVelocity3D.z != vel->z) {
            this->mFlags |= CHANNELI_FLAG_MOVED;
        }

        this->mVelocity3D = *vel;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_OK;
    }

    if (this->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_PAUSED) {
        this->update(0);
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->set3DAttributes();
        if (result == FMOD_OK) {
            result = result2;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::set3DConeOrientation(FMOD_VECTOR* orientation) {
    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (!orientation) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mConeOrientation = *orientation;

    this->mFlags |= CHANNELI_FLAG_MOVED;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::set3DConeSettings(float insideconeangle, float outsideconeangle, float outsidevolume) {
    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (insideconeangle > outsideconeangle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (outsidevolume > 1.0f) {
        outsidevolume = 1.0f;
    }

    if (outsidevolume < 0.0f) {
        outsidevolume = 0.0f;
    }

    this->mFlags |= CHANNELI_FLAG_MOVED;

    this->mConeInsideAngle = insideconeangle;
    this->mConeOutsideAngle = outsideconeangle;
    this->mConeOutsideVolume = outsidevolume;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setCallback(FMOD_CHANNEL_CALLBACKTYPE type, FMOD_CHANNEL_CALLBACK callback, int32_t command) {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (type >= FMOD_CHANNEL_CALLBACKTYPE_MAX) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mCallback[type] = callback;
    this->mCallbackCommand[type] = command;

    if (type == FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE && this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE]) {
        bool isvirtual = false;

        result = this->isVirtual(&isvirtual);
        if (result != FMOD_OK) {
            return result;
        }

        this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE](
            reinterpret_cast<FMOD_CHANNEL*>(static_cast<FMOD_UINT_NATIVE>(this->mHandleCurrent)),
            FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE,
            command,
            isvirtual ? 1 : 0,
            0);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setChannelGroup(ChannelGroupI* channelgroup) {
    return this->setChannelGroupInternal(channelgroup, true);
}

FMOD_RESULT ChannelI::setChannelGroupInternal(ChannelGroupI* channelgroup, bool resetattributes) {
    FMOD_RESULT result = FMOD_OK;
    auto oldchannelgroup = this->mChannelGroup;

    if (this->mChannelGroup) {
        if (this->mChannelGroupNode.isEmpty()) {
            return FMOD_OK;
        }

        this->mChannelGroup->mNumChannels--;
        this->mChannelGroupNode.removeNode();
    }

    if (!channelgroup) {
        channelgroup = this->mSystem->mChannelGroup;
    }

    this->mChannelGroup = channelgroup;
    this->mChannelGroupNode.addAfter(&this->mChannelGroup->mChannelHead);
    this->mChannelGroupNode.setData(this);
    this->mChannelGroup->mNumChannels++;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (resetattributes) {
        float levels[DSP_MAXLEVELS_OUT][DSP_MAXLEVELS_IN];

        if (this->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERLEVELS) {
            for (int32_t i = 0; i < this->mSystem->mMaxOutputChannels; i++) {
                this->getSpeakerLevels(static_cast<FMOD_SPEAKER>(i), levels[i], this->mSystem->mMaxInputChannels);
            }
        }

        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            FMOD_RESULT result2;

            result2 = this->mRealChannel[i]->moveChannelGroup(oldchannelgroup, this->mChannelGroup);
            if (result == FMOD_OK) {
                result = result2;
            }
        }

        if (this->mChannelGroup->mMute) {
            this->setMute(this->mChannelGroup->mMute);
        }

        if (this->mChannelGroup->mPaused) {
            this->setPaused(this->mChannelGroup->mPaused);
        }

        this->setVolume(this->mVolume, false);

        if (this->mRealChannel[0]->mMode & FMOD_3D) {
            if (this->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_PAUSED) {
                this->update(0);
            }
        } else {
            if (this->mLastPanMode == FMOD_CHANNEL_PANMODE_PAN) {
                this->setPan(this->mPan, true);
            } else if (this->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERMIX) {
                this->setSpeakerMix(
                    this->mSpeakerFL,
                    this->mSpeakerFR,
                    this->mSpeakerC,
                    this->mSpeakerLFE,
                    this->mSpeakerBL,
                    this->mSpeakerBR,
                    this->mSpeakerSL,
                    this->mSpeakerSR,
                    true);
            } else if (this->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERLEVELS) {
                for (int32_t i = 0; i < this->mSystem->mMaxOutputChannels; i++) {
                    this->setSpeakerLevels(static_cast<FMOD_SPEAKER>(i), levels[i], this->mSystem->mMaxInputChannels, true);
                }
            }
        }

        this->setFrequency(this->mFrequency);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setChannelInfo(FMOD_CHANNEL_INFO* info) {
    this->setMode(info->mMode);

    this->setVolume(this->mVolume, false);
    this->setFrequency(this->mFrequency);

    if (this->mLastPanMode == FMOD_CHANNEL_PANMODE_PAN) {
        this->setPan(this->mPan, true);

    } else if (this->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERMIX) {
        this->setSpeakerMix(
            this->mSpeakerFL,
            this->mSpeakerFR,
            this->mSpeakerC,
            this->mSpeakerLFE,
            this->mSpeakerBL,
            this->mSpeakerBR,
            this->mSpeakerSL,
            this->mSpeakerSR,
            true);

    } else if (this->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERLEVELS) {
        this->mLevels = info->mLevels;

        if (this->mLevels) {
            for (int32_t i = 0; i < this->mSystem->mMaxOutputChannels; i++) {
                this->setSpeakerLevels(
                    static_cast<FMOD_SPEAKER>(i),
                    &this->mLevels[i * this->mSystem->mMaxOutputChannels],
                    this->mSystem->mMaxInputChannels,
                    true);
            }
        }
    }

    this->set3DAttributes(&this->mPosition3D, &this->mVelocity3D);

    this->setDelay(info->mDelayStart, info->mDelayEnd);
    this->setPosition(info->mPCM, FMOD_TIMEUNIT_PCM);

    this->setLoopPoints(info->mLoopStart, FMOD_TIMEUNIT_PCM, info->mLoopEnd, FMOD_TIMEUNIT_PCM);
    this->setLoopCount(info->mLoopCount);

    this->setMute(info->mMute);

    this->setReverbProperties(&info->mReverbProperties);

    if (info->mAddDSP) {
        this->addDSPChain(info->mAddDSP);
    }

    if (this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE]) {
        bool isvirtual;
        this->isVirtual(&isvirtual);

        this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE](
            reinterpret_cast<FMOD_CHANNEL*>(this->mHandleCurrent),
            FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE,
            this->mCallbackCommand[FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE],
            isvirtual,
            0);
    }

    this->update(0);

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setDefaults() {
    float frequencyvariation;
    float panvariation;
    float realfrequency;
    float realpan;
    float realvolume;
    float volumevariation;
    int32_t channels;
    int32_t defaultpriority;
    uint32_t channelmask;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mRealChannel[0]->mSound) {
        auto soundi = this->mRealChannel[0]->mSound;

        realfrequency = soundi->mDefaultFrequency;
        channels = soundi->mChannels;
        defaultpriority = soundi->mDefaultPriority;
        realvolume = soundi->mDefaultVolume;
        realpan = soundi->mDefaultPan;
        frequencyvariation = soundi->mFrequencyVariation;
        volumevariation = soundi->mVolumeVariation;
        panvariation = soundi->mPanVariation;
        channelmask = soundi->mDefaultChannelMask;
    } else if (this->mRealChannel[0]->mDSP) {
        auto dspi = this->mRealChannel[0]->mDSP;

        realfrequency = dspi->mDefaultFrequency;
        defaultpriority = dspi->mDefaultPriority;
        channels = 0;
        realvolume = dspi->mDefaultVolume;
        channelmask = 0;
        realpan = dspi->mDefaultPan;
        frequencyvariation = 0.0f;
        volumevariation = 0.0f;
        panvariation = 0.0f;
    } else {
        return FMOD_ERR_INTERNAL;
    }

    this->mPriority = defaultpriority;
    this->mDirectOcclusion = 0.0f;
    this->mDirectOcclusionTarget = 0.0f;
    this->mReverbOcclusion = 0.0f;
    this->mReverbOcclusionTarget = 0.0f;

    if (frequencyvariation > 0.0f) {
        realfrequency += frequencyvariation * FMOD_Random_Value(1.0f);
    }

    if (volumevariation > 0.0f) {
        realvolume += volumevariation * FMOD_Random_Value(1.0f);
    }

    if (panvariation > 0.0f) {
        realpan += panvariation * FMOD_Random_Value(2.0f);
    }

    this->setFrequency(realfrequency);
    this->setVolume(realvolume, false);

    if (!(channelmask & SPEAKER_WAVEFORMAT_MASK)) {
        this->setPan(realpan, true);
    } else {
        float clevels[DSP_MAXLEVELS_IN];
        int32_t incount = 0;
        uint32_t maskbit = 1;

        for (int32_t speaker = 0; speaker < this->mSystem->mMaxOutputChannels; speaker++) {
            if (channelmask & maskbit) {
                FMOD_memset(clevels, 0, sizeof(clevels));
                clevels[incount] = 1.0f;
                this->setSpeakerLevels(static_cast<FMOD_SPEAKER>(speaker), clevels, channels, true);
                incount++;
            }

            maskbit <<= 1;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setDelay(uint32_t startdelay, uint32_t enddelay) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->setDelay(startdelay, enddelay);
        if (result == FMOD_OK) {
            result = result2;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setFrequency(float frequency) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (frequency < this->mRealChannel[0]->mMinFrequency) {
        frequency = this->mRealChannel[0]->mMinFrequency;
    }

    if (frequency > this->mRealChannel[0]->mMaxFrequency) {
        frequency = this->mRealChannel[0]->mMaxFrequency;
    }

    this->mFrequency = frequency;

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->setFrequency(this->mFrequency);
        if (result == FMOD_OK) {
            result = result2;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setLoopCount(int32_t loopcount) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->setLoopCount(loopcount);
        if (result == FMOD_OK) {
            result = result2;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setLoopPoints(uint32_t loopstart, FMOD_TIMEUNIT loopstarttype, uint32_t loopend, FMOD_TIMEUNIT loopendtype) {
    FMOD_RESULT result = FMOD_OK;
    uint32_t loopstartpcm = 0;
    uint32_t loopendpcm = 0;
    uint32_t looplengthpcm = 0;
    SoundI* soundi;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if ((loopstarttype != FMOD_TIMEUNIT_MS &&
         loopstarttype != FMOD_TIMEUNIT_PCM &&
         loopstarttype != FMOD_TIMEUNIT_PCMBYTES) ||
        (loopendtype != FMOD_TIMEUNIT_MS &&
         loopendtype != FMOD_TIMEUNIT_PCM &&
         loopendtype != FMOD_TIMEUNIT_PCMBYTES)) {
        return FMOD_ERR_FORMAT;
    }

    soundi = static_cast<SoundI*>(this->mRealChannel[0]->mSound);
    if (!soundi) {
        return FMOD_ERR_INVALID_PARAM;
    }

    soundi = soundi->mSubSampleParent;

    if (loopstarttype == FMOD_TIMEUNIT_PCM) {
        loopstartpcm = loopstart;
    } else if (loopstarttype == FMOD_TIMEUNIT_PCMBYTES) {
        SoundI::getSamplesFromBytes(loopstart, &loopstartpcm, soundi->mChannels, soundi->mFormat);
    } else if (loopstarttype == FMOD_TIMEUNIT_MS) {
        loopstartpcm = static_cast<uint32_t>(static_cast<float>(loopstart) / 1000.0f * soundi->mDefaultFrequency);
    }

    if (loopendtype == FMOD_TIMEUNIT_PCM) {
        loopendpcm = loopend;
    } else if (loopendtype == FMOD_TIMEUNIT_PCMBYTES) {
        SoundI::getSamplesFromBytes(loopend, &loopendpcm, soundi->mChannels, soundi->mFormat);
    } else if (loopendtype == FMOD_TIMEUNIT_MS) {
        loopendpcm = static_cast<uint32_t>(static_cast<float>(loopend) / 1000.0f * soundi->mDefaultFrequency);
    }

    if (loopstartpcm >= loopendpcm) {
        return FMOD_ERR_INVALID_PARAM;
    }

    looplengthpcm = loopendpcm - loopstartpcm + 1;

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->setLoopPoints(loopstartpcm, looplengthpcm);
        if (result == FMOD_OK) {
            result = result2;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setMute(bool mute) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (mute) {
        this->mFlags |= CHANNELI_FLAG_MUTED;
    } else {
        this->mFlags &= ~CHANNELI_FLAG_MUTED;
    }

    auto current = this->mChannelGroup;
    do {
        if (current->mMute) {
            mute = true;
            break;
        }
        current = current->mParent;
    } while (current);

    if (mute) {
        this->mFlags |= CHANNELI_FLAG_REALMUTE;

        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            FMOD_RESULT result2;

            result2 = this->mRealChannel[i]->setVolume(0.0f);
            if (result == FMOD_OK) {
                result = result2;
            }
        }
    } else {
        this->mFlags &= ~CHANNELI_FLAG_REALMUTE;

        result = this->setVolume(this->mVolume, false);
    }

    return result;
}

FMOD_RESULT ChannelI::setPan(float pan, bool calldriver) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (pan < -1.0f) {
        pan = -1.0f;
    }

    if (pan > 1.0f) {
        pan = 1.0f;
    }

    this->mPan = pan;

    this->mLastPanMode = FMOD_CHANNEL_PANMODE_PAN;

    if (this->mRealChannel[0]->mMode & FMOD_3D) {
        return FMOD_OK;
    } else if (calldriver) {
        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            FMOD_RESULT result2;

            if (this->mNumRealChannels > 1) {
                if (this->mNumRealChannels == 2 || (this->mRealChannel[0]->mSound && this->mRealChannel[0]->mSound->mDefaultChannelMask == SPEAKER_ALLSTEREO)) {
                    if (!(i & 1)) {
                        pan = -1.0f;
                    } else {
                        pan = 1.0f;
                    }
                }
            }

            result2 = this->mRealChannel[i]->setPan(pan, 1.0f);
            if (result == FMOD_OK) {
                result = result2;
            }
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setPaused(bool paused) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (paused) {
        this->mFlags |= CHANNELI_FLAG_PAUSED;
    } else {
        this->mFlags &= ~CHANNELI_FLAG_PAUSED;
    }

    auto current = this->mChannelGroup;
    do {
        if (current->mPaused) {
            paused = true;
            break;
        }
        current = current->mParent;
    } while (current);

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->setPaused(paused);
        if (result == FMOD_OK) {
            result = result2;
        }

        if (paused) {
            this->mRealChannel[i]->mFlags |= CHANNELREAL_FLAG_PAUSED;
        } else {
            this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_PAUSED;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setPosition(uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mRealChannel[0]->mSound) {
        uint32_t length;

        auto soundi = this->mRealChannel[0]->mSound->mSubSampleParent;

        if (postype == FMOD_TIMEUNIT_SENTENCE_MS ||
            postype == FMOD_TIMEUNIT_SENTENCE_PCM ||
            postype == FMOD_TIMEUNIT_SENTENCE_PCMBYTES ||
            postype == FMOD_TIMEUNIT_SENTENCE_SUBSOUND) {
            uint32_t sentenceid;
            uint32_t subsound;

            if (!soundi->mSubSoundList) {
                return FMOD_ERR_INVALID_PARAM;
            }

            if (postype == FMOD_TIMEUNIT_SENTENCE_SUBSOUND) {
                if (position >= (unsigned int)soundi->mSubSoundListNum) {
                    return FMOD_ERR_INVALID_PARAM;
                }

                sentenceid = position;
                postype = FMOD_TIMEUNIT_PCM;
                position = 0;
            } else {
                result = this->getPosition(&sentenceid, FMOD_TIMEUNIT_SENTENCE);
                if (result != FMOD_OK) {
                    return result;
                }
            }

            subsound = soundi->mSubSoundList[sentenceid];

            if (postype == FMOD_TIMEUNIT_SENTENCE_MS) {
                postype = FMOD_TIMEUNIT_MS;
            } else if (postype == FMOD_TIMEUNIT_SENTENCE_PCM) {
                postype = FMOD_TIMEUNIT_PCM;
            } else if (postype == FMOD_TIMEUNIT_SENTENCE_PCMBYTES) {
                postype = FMOD_TIMEUNIT_PCMBYTES;
            }

            if (soundi->mSubSoundShared) {
                FMOD_CODEC_WAVEFORMAT waveformat;

                soundi->mCodec->mDescription.getwaveformat(soundi->mCodec, subsound, &waveformat);

                length = waveformat.lengthpcm;

                if (postype == FMOD_TIMEUNIT_MS) {
                    position = static_cast<uint32_t>(static_cast<float>(position) / 1000.0f * waveformat.frequency);
                } else if (postype == FMOD_TIMEUNIT_PCMBYTES) {
                    SoundI::getSamplesFromBytes(position, &position, waveformat.channels, waveformat.format);
                }

                postype = FMOD_TIMEUNIT_PCM;
            } else {
                result = soundi->mSubSound[subsound]->getLength(&length, postype);
                if (result != FMOD_OK) {
                    return result;
                }
            }

            if (position >= length) {
                return FMOD_ERR_INVALID_PARAM;
            }

            for (int32_t i = 0; i < sentenceid; i++) {
                if (soundi->mSubSoundShared) {
                    FMOD_CODEC_WAVEFORMAT waveformat;

                    soundi->mCodec->mDescription.getwaveformat(soundi->mCodec, soundi->mSubSoundList[i], &waveformat);

                    length = waveformat.lengthpcm;
                } else {
                    auto sound = soundi->mSubSound[soundi->mSubSoundList[i]];

                    sound->getLength(&length, postype);
                }

                position += length;
            }

        } else {
            result = soundi->getLength(&length, postype);
            if (result != FMOD_OK) {
                return result;
            }

            if (length <= position) {
                return FMOD_ERR_INVALID_PARAM;
            }
        }
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->setPosition(position, postype);
        if (result2 != FMOD_OK && result2 != FMOD_ERR_INVALID_PARAM) {
            return result2;
        }
    }

    result = this->updateSyncPoints(true);
    if (result != FMOD_OK) {
        return result;
    }

    return result;
}

FMOD_RESULT ChannelI::setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        FMOD_RESULT result2;

        result2 = this->mRealChannel[i]->setReverbProperties(prop);
        if (result == FMOD_OK) {
            result = result2;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setSpeakerLevels(FMOD_SPEAKER speaker, float* levels, int32_t numlevels, bool calldriver) {
    FMOD_RESULT result = FMOD_OK;
    float actuallevels[DSP_MAXLEVELS_IN];

    if (!levels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mSystem->mMaxInputChannels < numlevels) {
        return FMOD_ERR_TOOMANYCHANNELS;
    }

    if (speaker >= FMOD_SPEAKER_MAX) {
        return FMOD_ERR_INVALID_PARAM;
    }

    switch (this->mSystem->mSpeakerMode) {
    case FMOD_SPEAKERMODE_MONO: {
        if (speaker != FMOD_SPEAKER_FRONT_LEFT) {
            return FMOD_ERR_INVALID_SPEAKER;
        }
        break;
    }
    case FMOD_SPEAKERMODE_STEREO: {
        if (speaker != FMOD_SPEAKER_FRONT_LEFT &&
            speaker != FMOD_SPEAKER_FRONT_RIGHT) {
            return FMOD_ERR_INVALID_SPEAKER;
        }
        break;
    }
    case FMOD_SPEAKERMODE_QUAD: {
        if (speaker != FMOD_SPEAKER_FRONT_LEFT &&
            speaker != FMOD_SPEAKER_FRONT_RIGHT &&
            speaker != FMOD_SPEAKER_BACK_LEFT &&
            speaker != FMOD_SPEAKER_BACK_RIGHT) {
            return FMOD_ERR_INVALID_SPEAKER;
        }

        if (speaker == FMOD_SPEAKER_BACK_LEFT) {
            speaker = static_cast<FMOD_SPEAKER>(2);
        }
        if (speaker == FMOD_SPEAKER_BACK_RIGHT) {
            speaker = static_cast<FMOD_SPEAKER>(3);
        }
        break;
    }
    case FMOD_SPEAKERMODE_SURROUND: {
        if (speaker != FMOD_SPEAKER_FRONT_LEFT &&
            speaker != FMOD_SPEAKER_FRONT_RIGHT &&
            speaker != FMOD_SPEAKER_BACK_LEFT &&
            speaker != FMOD_SPEAKER_BACK_RIGHT &&
            speaker != FMOD_SPEAKER_FRONT_CENTER) {
            return FMOD_ERR_INVALID_SPEAKER;
        }
        break;
    }
    case FMOD_SPEAKERMODE_5POINT1:
    case FMOD_SPEAKERMODE_PROLOGIC:
        if (speaker != FMOD_SPEAKER_FRONT_LEFT &&
            speaker != FMOD_SPEAKER_FRONT_RIGHT &&
            speaker != FMOD_SPEAKER_BACK_LEFT &&
            speaker != FMOD_SPEAKER_BACK_RIGHT &&
            speaker != FMOD_SPEAKER_FRONT_CENTER &&
            speaker != FMOD_SPEAKER_LOW_FREQUENCY) {
            return FMOD_ERR_INVALID_SPEAKER;
        }
        break;
    case FMOD_SPEAKERMODE_7POINT1: {
        if (speaker >= FMOD_SPEAKER_MAX) {
            return FMOD_ERR_INVALID_SPEAKER;
        }
    }
    default: {
        break;
    }
    }

    FMOD_memset(actuallevels, 0, sizeof(actuallevels));

    if (!this->mLevels) {
        this->mSystem->mSpeakerLevelsPool.alloc(&this->mLevels);
        if (!this->mLevels) {
            return FMOD_ERR_MEMORY;
        }
    }

    for (int32_t i = 0; i < numlevels; i++) {
        auto volume = levels[i];

        if (volume < 0.0f) {
            volume = 0.0f;
        }
        if (volume > 1.0f) {
            volume = 1.0f;
        }

        this->mLevels[(speaker * this->mSystem->mMaxInputChannels) + i] = volume;

        actuallevels[i] = volume;
    }

    this->mLastPanMode = FMOD_CHANNEL_PANMODE_SPEAKERLEVELS;

    if (this->mRealChannel[0]->mMode & FMOD_3D) {
        return FMOD_OK;
    } else if (calldriver) {
        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            FMOD_RESULT result2;

            result2 = this->mRealChannel[i]->setSpeakerLevels(speaker, actuallevels, numlevels);
            if (result == FMOD_OK) {
                result = result2;
            }
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setSpeakerMix(float frontleft, float frontright, float center, float lfe, float backleft, float backright, float sideleft, float sideright, bool calldriver) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    this->mSpeakerFL = frontleft < 0 ? 0 : frontleft > 5.0f ? 5.0f
                                                            : frontleft;
    this->mSpeakerFR = frontright < 0 ? 0 : frontright > 5.0f ? 5.0f
                                                              : frontright;
    this->mSpeakerC = center < 0 ? 0 : center > 5.0f ? 5.0f
                                                     : center;
    this->mSpeakerLFE = lfe < 0 ? 0 : lfe > 5.0f ? 5.0f
                                                 : lfe;
    this->mSpeakerBL = backleft < 0 ? 0 : backleft > 5.0f ? 5.0f
                                                          : backleft;
    this->mSpeakerBR = backright < 0 ? 0 : backright > 5.0f ? 5.0f
                                                            : backright;
    this->mSpeakerSL = sideleft < 0 ? 0 : sideleft > 5.0f ? 5.0f
                                                          : sideleft;
    this->mSpeakerSR = sideright < 0 ? 0 : sideright > 5.0f ? 5.0f
                                                            : sideright;
    this->mLastPanMode = FMOD_CHANNEL_PANMODE_SPEAKERMIX;

    if (this->mRealChannel[0]->mMode & FMOD_3D) {
        return FMOD_OK;
    } else if (calldriver) {

        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            FMOD_RESULT result2;

            result2 = this->mRealChannel[i]->setSpeakerMix(
                this->mSpeakerFL,
                this->mSpeakerFR,
                this->mSpeakerC,
                this->mSpeakerLFE,
                this->mSpeakerBL,
                this->mSpeakerBR,
                this->mSpeakerSL,
                this->mSpeakerSR);
            if (result == FMOD_OK) {
                result = result2;
            }
        }
    }

    return result;
}

FMOD_RESULT ChannelI::setVolume(float volume, bool forceupdatepos) {
    FMOD_RESULT result = FMOD_OK;
    bool updatepos = false;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (volume < 0.0f) {
        volume = 0.0f;
    }
    if (volume > 1.0f) {
        volume = 1.0f;
    }

    if (this->mVolume != volume) {
        updatepos = true;
    }

    this->mVolume = volume;

    if (this->mFlags & CHANNELI_FLAG_REALMUTE) {
        volume = 0.0f;
    }

    if (this->mNumRealChannels >= 2 && this->mLevels) {
        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            result = this->mRealChannel[i]->updateSpeakerLevels(volume);
        }
    } else {
        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            result = this->mRealChannel[i]->setVolume(volume);
        }
    }

    if (updatepos || forceupdatepos) {
        result = this->updatePosition();
        if (result != FMOD_OK) {
            return result;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::start() {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        result = this->mRealChannel[i]->start();
        if (result != FMOD_OK) {
            return result;
        }

        this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_STOPPED;
        this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ALLOCATED;
        this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ENDDELAY;
        this->mRealChannel[i]->mFlags |= CHANNELREAL_FLAG_PLAYING;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::stopEx(bool refstamp, bool updatelist, bool resetcallbacks, bool stoprealchannels, bool callendcallback, bool resetchannelgroup, bool processenddelay) {
    FMOD_RESULT result;
    ChannelReal* oldrealchannel[FMOD_CHANNEL_MAXREALSUBCHANNELS];

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_STOPPED)) {
        if (stoprealchannels) {
            for (int32_t i = 0; i < this->mNumRealChannels; i++) {
                this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_IN_USE;
                this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ALLOCATED;
                this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_PLAYING;
                this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_PAUSED;
                this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_PAUSEDFORSETPOS;
                this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ENDDELAY;
                this->mRealChannel[i]->mFlags |= CHANNELREAL_FLAG_STOPPED;
            }
        }

        if (resetchannelgroup) {
            result = this->setChannelGroupInternal(nullptr, false);
            if (result != FMOD_OK) {
                return result;
            }
        }

        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            result = this->mRealChannel[i]->stop(stoprealchannels, processenddelay);
            if (result != FMOD_OK) {
                return result;
            }

            if (this->mRealChannel[i]->mSound) {
                this->mRealChannel[i]->mSound->mNumAudible--;
            }
        }

        if (updatelist) {
            result = this->returnToFreeList();
            if (result != FMOD_OK) {
                return result;
            }

            this->mListPosition = static_cast<uint32_t>(-1);
        }

        this->mFlags &= ~CHANNELI_FLAG_JUSTWENTVIRTUAL;
        this->mFlags &= ~CHANNELI_FLAG_USEDADDDSP;
        this->mFlags &= ~CHANNELI_FLAG_USEDINPUTMIX;

        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            oldrealchannel[i] = this->mRealChannel[i];
        }

        if (callendcallback && this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_END]) {
            this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_END](
                reinterpret_cast<FMOD_CHANNEL*>(this->mHandleCurrent),
                FMOD_CHANNEL_CALLBACKTYPE_END,
                this->mCallbackCommand[0],
                0,
                0);
        }

        for (int32_t i = 0; i < this->mNumRealChannels; i++) {
            if (!(this->mRealChannel[i]->mFlags & CHANNELREAL_FLAG_PLAYING) || oldrealchannel[i] != this->mRealChannel[i]) {
                bool crit = false;

                if (oldrealchannel[i]->mSound && oldrealchannel[i]->mSound->isStream()) {
                    crit = true;
                }

                if (crit) {
                    FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamRealchanCrit);
                }

                oldrealchannel[i]->mSound = nullptr;
                oldrealchannel[i]->mDSP = nullptr;
                oldrealchannel[i]->mParent = nullptr;

                if (crit) {
                    FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamRealchanCrit);
                }
            }
        }

        if (this->mListPosition == static_cast<uint32_t>(-1)) {
            for (int32_t i = 0; i < this->mNumRealChannels; i++) {
                this->mRealChannel[i] = nullptr;
            }

            if (resetcallbacks) {
                for (int32_t i = 0; i < FMOD_CHANNEL_CALLBACKTYPE_MAX; i++) {
                    this->mCallback[i] = nullptr;
                    this->mCallbackCommand[i] = 0;
                }
                this->mSyncPointCurrent = nullptr;
            }

            if (refstamp) {
                result = this->referenceStamp(false);
                if (result != FMOD_OK) {
                    return result;
                }
            }
        }

        if (this->mLevels) {
            this->mSystem->mSpeakerLevelsPool.free(this->mLevels);
            this->mLevels = nullptr;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::updateSyncPoints(bool seeking) {
    FMOD_RESULT result;
    auto sound = this->mRealChannel[0]->mSound;

    if (sound) {
        if (sound->mSyncPointHead && sound->mNumSyncPoints && !this->mSyncPointCurrent) {
            this->mSyncPointCurrent = sound->mSyncPointHead;
        }

        if (this->mSyncPointCurrent) {
            uint32_t position;

            result = this->getPosition(&position, FMOD_TIMEUNIT_PCM);
            if (result == FMOD_OK) {
                auto parent = sound->mSubSampleParent;
                int32_t index;
                SyncPoint* candidate;

                if (this->mFrequency > 0) {
                    index = 0;
                    candidate = static_cast<SyncPoint*>(parent->mSyncPointHead->getNext());
                } else {
                    index = parent->mNumSyncPoints - 1;
                    candidate = static_cast<SyncPoint*>(parent->mSyncPointHead->getPrev());
                }

                if (candidate) {
                    if (seeking) {
                        while (true) {
                            bool crossed;
                            if (this->mFrequency > 0) {
                                crossed = (candidate->mOffset <= position);
                            } else {
                                crossed = (candidate->mOffset >= position);
                            }
                            if (!crossed) {
                                break;
                            }

                            this->mSyncPointCurrent = candidate;
                            if (this->mFrequency > 0) {
                                candidate = static_cast<SyncPoint*>(candidate->getNext());
                                index++;
                                if (index >= parent->mNumSyncPoints) {
                                    break;
                                }
                            } else {
                                candidate = static_cast<SyncPoint*>(candidate->getPrev());
                                index--;
                                if (index < 0) {
                                    break;
                                }
                            }
                        }
                    } else {
                        if ((this->mFrequency > 0 && position < this->mSyncPointCurrent->mOffset) ||
                            (this->mFrequency < 0 && position > this->mSyncPointCurrent->mOffset)) {
                            this->mSyncPointCurrent = parent->mSyncPointHead;
                        }

                        while (true) {
                            bool crossed;
                            if (this->mFrequency > 0) {
                                crossed = (candidate->mOffset > this->mSyncPointCurrent->mOffset &&
                                           candidate->mOffset <= position);
                            } else {
                                crossed = (candidate->mOffset < this->mSyncPointCurrent->mOffset &&
                                           candidate->mOffset >= position);
                            }
                            if (!crossed) {
                                break;
                            }

                            if (this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_SYNCPOINT]) {
                                this->mCallback[FMOD_CHANNEL_CALLBACKTYPE_SYNCPOINT](
                                    reinterpret_cast<FMOD_CHANNEL*>(this->mHandleCurrent),
                                    FMOD_CHANNEL_CALLBACKTYPE_SYNCPOINT,
                                    this->mCallbackCommand[FMOD_CHANNEL_CALLBACKTYPE_SYNCPOINT],
                                    index,
                                    0);
                            }

                            this->mSyncPointCurrent = candidate;

                            if (this->mFrequency > 0) {
                                candidate = static_cast<SyncPoint*>(candidate->getNext());
                                index++;
                                if (index >= parent->mNumSyncPoints) {
                                    break;
                                }
                            } else {
                                candidate = static_cast<SyncPoint*>(candidate->getPrev());
                                index--;
                                if (index < 0) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::stop() {
    return this->stopEx(true, true, true, true, true, true, false);
}

FMOD_RESULT ChannelI::forceVirtual(bool force) {
    FMOD_RESULT result;

    if (force) {
        bool isvirtual;

        if (this->mFlags & CHANNELI_FLAG_FORCEVIRTUAL) {
            return FMOD_OK;
        }

        result = this->isVirtual(&isvirtual);
        if (result != FMOD_OK) {
            return result;
        }

        mFlags |= CHANNELI_FLAG_FORCEVIRTUAL;

        if (!isvirtual && this->mRealChannel[0]) {
            FMOD_CHANNEL_INFO realinfo;
            ChannelReal* emuchannelreal;
            ChannelGroupI* channelgroup;

            result = this->mSystem->mEmulated->getFreeChannel(this->mRealChannel[0]->mMode, &emuchannelreal, 1, 1, 0);
            if (result != FMOD_OK) {
                return result;
            }

            channelgroup = this->mChannelGroup;

            this->getChannelInfo(&realinfo);
            this->stopEx(false, false, false, true, false, true, false);

            this->mNumRealChannels = 1;
            this->mRealChannel[0] = emuchannelreal;

            if (realinfo.mSound) {
                this->play(realinfo.mSound->mSubSampleParent, true, false, false);
                this->setChannelGroup(channelgroup);
                this->setChannelInfo(&realinfo);
                this->setPaused(realinfo.mPaused);
            } else if (realinfo.mDSP) {
                this->play(realinfo.mDSP, true, false, false);
                this->setChannelGroup(channelgroup);
                this->setChannelInfo(&realinfo);
                this->setPaused(realinfo.mPaused);
            }
        }
    } else if (this->mFlags & CHANNELI_FLAG_FORCEVIRTUAL) {
        this->mFlags &= ~CHANNELI_FLAG_FORCEVIRTUAL;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::updatePosition() {
    uint32_t oldlistposition;
    float audibilitysoundgroup;
    bool forcevirtual = false;
    float realdirecttransmission, realreverbtransmission;

    if (!this->mSystem) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    this->getAudibility(&audibilitysoundgroup);

    realdirecttransmission = (1.0f - this->mDirectOcclusion) * this->mChannelGroup->mRealDirectOcclusionVolume;
    realreverbtransmission = (1.0f - this->mReverbOcclusion) * this->mChannelGroup->mRealReverbOcclusionVolume;

    if (audibilitysoundgroup <= this->mSystem->mAdvancedSettings.vol0virtualvol && this->mSystem->mFlags & FMOD_INIT_VOL0_BECOMES_VIRTUAL) {
        forcevirtual = true;
    }

    if (realdirecttransmission == 0.0f && realreverbtransmission > 0.0f) {
        forcevirtual = false;
    }

    this->forceVirtual(forcevirtual);

    oldlistposition = this->mListPosition;

    this->mListPosition = this->mPriority * (FMOD_MAXAUDIBIILITY + 1);
    this->mListPosition += (FMOD_MAXAUDIBIILITY - static_cast<int32_t>(audibilitysoundgroup * FMOD_MAXAUDIBIILITY));

    if (this->mListPosition != oldlistposition) {
        this->mSortedListNode.removeNode();
        this->mSortedListNode.addAt(&this->mSystem->mChannelSortedListHead, this->mListPosition);
        this->mSortedListNode.setData(this);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setPriority(int32_t priority) {
    if (priority < 0 || priority > FMOD_MAXPRIORITIES) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mPriority = priority;

    return updatePosition();
}

FMOD_RESULT ChannelI::set3DOcclusionInternal(float direct, float reverb, bool resettarget) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (direct < 0.0f) {
        direct = 0.0f;
    }
    if (reverb < 0.0f) {
        reverb = 0.0f;
    }
    if (direct > 1.0f) {
        direct = 1.0f;
    }
    if (reverb > 1.0f) {
        reverb = 1.0f;
    }

    this->mDirectOcclusion = direct;
    this->mReverbOcclusion = reverb;

    if (resettarget) {
        this->mDirectOcclusionTarget = this->mDirectOcclusion;
        this->mReverbOcclusionTarget = this->mReverbOcclusion;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        result = this->mRealChannel[i]->set3DOcclusion(direct, reverb);
    }

    result = this->updatePosition();
    if (result != FMOD_OK) {
        return result;
    }

    return result;
}

FMOD_RESULT ChannelI::update(int32_t delta) {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        bool playingpaused = (this->mRealChannel[i]->mFlags & CHANNELREAL_FLAG_PLAYING) &&
                             (this->mRealChannel[i]->mFlags & CHANNELREAL_FLAG_PAUSED);

        if ((this->mRealChannel[i]->mMode & FMOD_3D) && !(this->mRealChannel[i]->mMode & FMOD_3D_IGNOREGEOMETRY)) {
            // TODO: GeometryMgr
            if (((this->mFlags & CHANNELI_FLAG_MOVED) ||
                 this->mSystem->mListener[0].mMoved ||
                 // this->mSystem->mGeometryMgr.mMoved ||
                 playingpaused)
                // && this->mSystem->mGeometryList
            ) {
                FMOD_VECTOR channelpos;
                float gain;
                float lingain;

                if (this->mRealChannel[0]->mMode & FMOD_3D_HEADRELATIVE) {
                    FMOD_Vector_Add(&this->mPosition3D, &this->mSystem->mListener[0].mPosition, &channelpos);
                } else {
                    FMOD_Vector_Copy(&this->mPosition3D, &channelpos);
                }

                // TODO:
                // this->mSystem->mGeometryMgr.lineTestAll(&this->mSystem->mListener[0].mPosition, &channelpos, &this->mDirectOcclusionTarget, &this->mReverbOcclusionTarget);
                // WORKAROUND:
                this->mDirectOcclusionTarget = 0.0f;
                this->mReverbOcclusionTarget = 0.0f;

                this->calculate3DReverbGain(&this->mSystem->mReverb3D, &channelpos, &gain);
                this->mSystem->mReverb3D.getPresenceGain(this->mIndex, &lingain);
                if (gain != lingain) {
                    this->mSystem->mReverb3D.setPresenceGain(this->mIndex, gain);
                }

                auto reverb = static_cast<ReverbI*>(this->mSystem->mReverb3DHead.getNext());
                while (reverb != &this->mSystem->mReverb3DHead) {
                    if (reverb->getMode() == FMOD_REVERB_PHYSICAL) {
                        float gain;
                        float lingain;
                        this->calculate3DReverbGain(reverb, &channelpos, &gain);
                        reverb->getPresenceGain(this->mIndex, &lingain);
                        if (gain != lingain) {
                            reverb->setPresenceGain(this->mIndex, gain);
                        }
                    }

                    reverb = static_cast<ReverbI*>(reverb->getNext());
                }
            }

            if ((this->mDirectOcclusionTarget != this->mDirectOcclusion || this->mReverbOcclusionTarget != this->mReverbOcclusion) && delta) {
                if (playingpaused) {
                    this->mDirectOcclusion = this->mDirectOcclusionTarget;
                    this->mReverbOcclusion = this->mReverbOcclusionTarget;
                } else if (this->mDirectOcclusionTarget <= this->mDirectOcclusion) {
                    if (this->mDirectOcclusionTarget < this->mDirectOcclusion) {
                        this->mDirectOcclusion -= (static_cast<float>(delta) * 0.002f);
                        if (this->mDirectOcclusion < this->mDirectOcclusionTarget) {
                            this->mDirectOcclusion = this->mDirectOcclusionTarget;
                        }
                    }
                } else {
                    this->mDirectOcclusion += (static_cast<float>(delta) * 0.002f);
                    if (this->mDirectOcclusion > this->mDirectOcclusionTarget) {
                        this->mDirectOcclusion = this->mDirectOcclusionTarget;
                    }
                }
                if (this->mReverbOcclusionTarget <= this->mReverbOcclusion) {
                    if (this->mReverbOcclusionTarget < this->mReverbOcclusion) {
                        this->mReverbOcclusion -= (static_cast<float>(delta) * 0.002f);
                        if (this->mReverbOcclusion < this->mReverbOcclusionTarget)
                            this->mReverbOcclusion = this->mReverbOcclusionTarget;
                    }
                } else {
                    this->mReverbOcclusion += (static_cast<float>(delta) * 0.002f);
                    if (this->mReverbOcclusion > this->mReverbOcclusionTarget) {
                        this->mReverbOcclusion = this->mReverbOcclusionTarget;
                    }
                }
                this->set3DOcclusionInternal(this->mDirectOcclusion, this->mReverbOcclusion, false);
            }
        }
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        if (this->mRealChannel[i]->mDelayEnd && this->mRealChannel[i]->mFlags & CHANNELREAL_FLAG_ENDDELAY) {
            if (static_cast<uint32_t>(delta) < this->mRealChannel[i]->mDelayEnd) {
                this->mRealChannel[i]->mDelayEnd -= delta;
            } else {
                this->mRealChannel[i]->mDelayEnd = 0;
                this->mRealChannel[i]->mFlags &= ~CHANNELREAL_FLAG_ENDDELAY;
            }
        }
    }

    result = this->calcVolumeAndPitchFor3D(delta);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        result = this->mRealChannel[i]->set2DFreqVolumePanFor3D();
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->updateSyncPoints(false);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_STOPPED) {
        return FMOD_OK;
    }

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        result = this->mRealChannel[i]->update(delta);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if ((this->mFlags & CHANNELI_FLAG_MOVED) || this->mSystem->mListener[0].mMoved) {
        result = this->updatePosition();
        if (result != FMOD_OK) {
            return result;
        }

        mFlags &= ~CHANNELI_FLAG_MOVED;
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::set3DMinMaxDistance(float mindistance, float maxdistance) {
    FMOD_RESULT result = FMOD_OK;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    auto mode = this->mRealChannel[0]->mMode;

    if (!(mode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (mindistance < 0 || maxdistance < 0 || maxdistance < mindistance) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mMinDistance != mindistance ||
        this->mMaxDistance != maxdistance) {
        this->mMinDistance = mindistance;
        this->mMaxDistance = maxdistance;

        if (mode & (FMOD_3D_LOGROLLOFF | FMOD_3D_LINEARROLLOFF | FMOD_3D_CUSTOMROLLOFF) || this->mSystem->mRolloffCallback) {
            this->mFlags |= CHANNELI_FLAG_MOVED;

            result = this->update(0);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->setVolume(this->mVolume, false);
            if (result != FMOD_OK) {
                return result;
            }
        } else {
            for (int32_t i = 0; i < this->mNumRealChannels; i++) {
                FMOD_RESULT result2;

                result2 = this->mRealChannel[i]->set3DMinMaxDistance();
                if (result == FMOD_OK) {
                    result = result2;
                }
            }

            this->mFlags |= CHANNELI_FLAG_MOVED;
        }
    }

    return result;
}

FMOD_RESULT ChannelI::set3DPanLevel(float level) {
    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!(this->mRealChannel[0]->mMode & FMOD_3D)) {
        return FMOD_ERR_NEEDS3D;
    }

    if (level < 0.0f || level > 1.0f) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->m3DPanLevel = level;

    if (this->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_PAUSED && this->m3DPanLevel < 1.0f) {
        return this->update(0);
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setMode(FMOD_MODE mode) {
    FMOD_RESULT result;

    if (!this->mRealChannel[0]) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    auto oldmode = this->mRealChannel[0]->mMode;

    for (int32_t i = 0; i < this->mNumRealChannels; i++) {
        result = this->mRealChannel[i]->setMode(mode);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (this->mRealChannel[0]->mMode & FMOD_SOFTWARE) {
        if (!(oldmode & FMOD_2D) && mode & FMOD_2D) {
            uint32_t channelmask = 0;
            int32_t channels = 0;

            result = this->setVolume(this->mVolume, false);
            if (result != FMOD_OK) {
                return result;
            }

            if (this->mRealChannel[0]->mSound) {
                auto soundi = this->mRealChannel[0]->mSound;
                channelmask = soundi->mDefaultChannelMask;
                channels = soundi->mChannels;
            }

            if (channelmask & SPEAKER_WAVEFORMAT_MASK) {
                int32_t incount = 0;
                uint32_t maskbit = 1;

                for (int32_t speaker = 0; speaker < mSystem->mMaxOutputChannels; speaker++) {
                    if (channelmask & maskbit) {
                        float clevels[DSP_MAXLEVELS_IN];
                        FMOD_memset(clevels, 0, sizeof(clevels));
                        clevels[incount] = 1.0f;

                        this->setSpeakerLevels(static_cast<FMOD_SPEAKER>(speaker), clevels, channels, true);

                        incount++;
                        if (incount >= channels) {
                            break;
                        }
                    }

                    maskbit <<= 1;
                }
            } else {
                this->setPan(this->mPan, true);
            }
        } else if (!(oldmode & FMOD_3D) && mode & FMOD_3D) {
            FMOD_VECTOR pos = this->mPosition3D;
            this->mPosition3D.x += 1.0f;

            result = this->set3DAttributes(&pos, &this->mVelocity3D);
            if (result != FMOD_OK) {
                return result;
            }
        }
    } else {
        if ((mode & FMOD_3D) && (mode & (FMOD_3D_LOGROLLOFF | FMOD_3D_LINEARROLLOFF | FMOD_3D_CUSTOMROLLOFF) || this->mSystem->mRolloffCallback)) {
            result = this->set3DAttributes(&this->mPosition3D, &this->mVelocity3D);
            if (result != FMOD_OK) {
                return result;
            }

            this->mFlags |= CHANNELI_FLAG_MOVED;

            result = this->update(0);
            if (result != FMOD_OK) {
                return result;
            }

            result = this->setVolume(this->mVolume, false);
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT ChannelI::setUserData(void* userdata) {
    this->mUserData = userdata;
    return FMOD_OK;
}

FMOD_RESULT ChannelI::getUserData(void** userdata) {
    if (!userdata) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *userdata = this->mUserData;

    return FMOD_OK;
}

FMOD_RESULT ChannelI::validate(Channel* channel, ChannelI** channeli) {
    FMOD_RESULT result;
    SystemI* sys;

    auto handle = static_cast<uint32_t>(reinterpret_cast<FMOD_UINT_NATIVE>(channel));

    auto sysindex = (handle >> 28) & 0xF;
    auto index = (handle >> 16) & 0xFFF;
    auto refcount = handle & 0xFFFF;

    if (!channeli) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *channeli = nullptr;

    result = SystemI::getInstance(sysindex, &sys);
    if (result != FMOD_OK) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!sys->mChannel) {
        return FMOD_ERR_UNINITIALIZED;
    }

    auto tmpchanneli = &sys->mChannel[index];
    if (refcount && (tmpchanneli->mHandleCurrent != handle)) {
        if (((tmpchanneli->mHandleCurrent & 0xFFFF) - refcount) >= 2) {
            return FMOD_ERR_CHANNEL_STOLEN;
        } else {
            return FMOD_ERR_INVALID_HANDLE;
        }
    }

    *channeli = tmpchanneli;
    return FMOD_OK;
}

} // namespace FMOD
