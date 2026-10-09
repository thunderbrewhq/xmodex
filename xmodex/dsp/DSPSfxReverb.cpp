#include "xmodex/dsp/DSPSfxReverb.hpp"
#include "fmod.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/ASfxDsp.hpp"
#include <cstdio>

#define FMODLISTENER_MINLFREFERENCE 20.0f
#define FMODLISTENER_MAXLFREFERENCE 1000.0f
#define FMODLISTENER_DEFAULTLFREFERENCE 250.0f

#define FMODLISTENER_MINROOMLF -10000
#define FMODLISTENER_MAXROOMLF 0
#define FMODLISTENER_DEFAULTROOMLF 0

namespace FMOD {

FMOD_DSP_DESCRIPTION_EX dspsfxreverb;

FMOD_DSP_PARAMETERDESC dspsfxreverb_param[] = {
    { -10000.0f, 0.0f, 0.0f,
      "Dry Level", "mB", "Dry Level" },

    { I3DL2LISTENER_MINROOM, I3DL2LISTENER_MAXROOM, I3DL2LISTENER_DEFAULTROOM,
      "Room", "mB", "Room" },

    { I3DL2LISTENER_MINROOMHF, I3DL2LISTENER_MAXROOMHF, I3DL2LISTENER_DEFAULTROOMHF,
      "Room HF", "mB", "Room HF" },

    { I3DL2LISTENER_MINROOMROLLOFFFACTOR, I3DL2LISTENER_MAXROOMROLLOFFFACTOR, I3DL2LISTENER_DEFAULTROOMROLLOFFFACTOR,
      "Room Rolloff", "", "Room Rolloff Factor" },

    { I3DL2LISTENER_MINDECAYTIME, I3DL2LISTENER_MAXDECAYTIME, I3DL2LISTENER_DEFAULTDECAYTIME,
      "Decay Time", "s", "Decay Time" },

    { I3DL2LISTENER_MINDECAYHFRATIO, I3DL2LISTENER_MAXDECAYHFRATIO, I3DL2LISTENER_DEFAULTDECAYHFRATIO,
      "Decay HF Ratio", "", "Decay HF Ratio" },

    { I3DL2LISTENER_MINREFLECTIONS, I3DL2LISTENER_MAXREFLECTIONS, I3DL2LISTENER_DEFAULTREFLECTIONS,
      "Reflections", "mB", "Reflections" },

    { I3DL2LISTENER_MINREFLECTIONSDELAY, I3DL2LISTENER_MAXREFLECTIONSDELAY, I3DL2LISTENER_DEFAULTREFLECTIONSDELAY,
      "Reflect Delay", "", "Reflections Delay" },

    { I3DL2LISTENER_MINREVERB, I3DL2LISTENER_MAXREVERB, I3DL2LISTENER_DEFAULTREVERB,
      "Reverb", "mB", "Reverb" },

    { I3DL2LISTENER_MINREVERBDELAY, I3DL2LISTENER_MAXREVERBDELAY, I3DL2LISTENER_DEFAULTREVERBDELAY,
      "Reverb Delay", "s", "Reverb Delay" },

    { I3DL2LISTENER_MINDIFFUSION, I3DL2LISTENER_MAXDIFFUSION, I3DL2LISTENER_DEFAULTDIFFUSION,
      "Diffusion", "%", "Diffusion" },

    { I3DL2LISTENER_MINDENSITY, I3DL2LISTENER_MAXDENSITY, I3DL2LISTENER_DEFAULTDENSITY,
      "Density", "%", "Density" },

    { I3DL2LISTENER_MINHFREFERENCE, I3DL2LISTENER_MAXHFREFERENCE, I3DL2LISTENER_DEFAULTHFREFERENCE,
      "HF Reference", "Hz", "HF Reference" },

    { FMODLISTENER_MINROOMLF, FMODLISTENER_MAXROOMLF, FMODLISTENER_DEFAULTROOMLF,
      "Room LF", "mB", "Room LF" },

    { FMODLISTENER_MINLFREFERENCE, FMODLISTENER_MAXLFREFERENCE, FMODLISTENER_DEFAULTLFREFERENCE,
      "LF Reference", "Hz", "LF Reference" }

};

bool DSPSfxReverb::Calculate1stOrderLowpassCoeff(float gain, float cutoff, float sampleRate, float* pK) {
    if (gain > 1.0f) {
        gain = 1.0f;
    }

    if (gain == 1.0f) {
        *pK = 0.0f;
    } else {
        float Gain = gain * gain;

        float a = Gain - 1.0f;
        float b = 2.0f * (1.0f - Gain * FMOD_COS(2.0f * FMOD_PI * cutoff / sampleRate));
        float c = a;

        float b24ac = b * b - 4.0f * a * c;

        if (b24ac < 0) {
            b24ac = 0;
        }

        *pK = (-b + FMOD_SQRT(b24ac)) / (2.0f * a);
        if (*pK < 0.0f) {
            *pK = 0.0f;
        } else if (*pK > 0.99f) {
            *pK = 0.99f;
        }
    }

    return false;
}

bool DSPSfxReverb::CalculateShelfCoeffs(float gain, float cutoff, float sampleRate, float* a0, float* a1, float* a2, float* b1, float* b2) {
    float t1;
    float t2;
    float g1;
    float g2;

    float sqrtgainlin = FMOD_EXP(gain * 0.057564627f);
    t1 = t2 = FMOD_TAN(3.14159265f * cutoff / sampleRate);
    g1 = g2 = 1.414213562f;
    t2 /= sqrtgainlin;

    float denom = 1.0f / (1.0f + t2 * (g2 + t2));
    *a0 = (1.0f + t1 * (t1 + g1)) * denom;
    *a1 = (t1 * t1 - 1.0f) * denom * 2.0f;
    *a2 = (1.0f + t1 * (t1 - g1)) * denom;
    *b1 = -2.0f * (t2 * t2 - 1.0f) * denom;
    *b2 = -(1.0f + t2 * (t2 - g2)) * denom;

    return false;
}

bool DSPSfxReverb::SetDecayHFRatio(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->flDecayHFRatio < I3DL2LISTENER_MINDECAYHFRATIO) {
        pProps->flDecayHFRatio = I3DL2LISTENER_MINDECAYHFRATIO;
    } else if (pProps->flDecayHFRatio > I3DL2LISTENER_MAXDECAYHFRATIO) {
        pProps->flDecayHFRatio = I3DL2LISTENER_MAXDECAYHFRATIO;
    }

    this->mProps.flDecayHFRatio = pProps->flDecayHFRatio;

    bool err = this->SetDecayTime(pProps);

    return err;
}

bool DSPSfxReverb::SetDecayTime(I3DL2_LISTENERPROPERTIES* pProps) {
    constexpr float NEW_MAX_DECAY_TIME = 10000.0f;
    float K;
    bool err = false;

    if (pProps->flDecayTime < I3DL2LISTENER_MINDECAYTIME) {
        pProps->flDecayTime = I3DL2LISTENER_MINDECAYTIME;
    } else if (pProps->flDecayTime > NEW_MAX_DECAY_TIME) {
        pProps->flDecayTime = NEW_MAX_DECAY_TIME;
    }

    this->mProps.flDecayTime = pProps->flDecayTime;

    for (int32_t delayLine = 0; delayLine < this->mpSfxDsp->mNumLateReverbDelays; delayLine++) {
        if (pProps->flDecayTime) {
            float attenuationDB = (-60.f / pProps->flDecayTime) * this->mpSfxDsp->mLateDelayLenSec[delayLine];
            float feedback = FMOD_POW(10.0f, attenuationDB / 20.0f);
            this->mpSfxDsp->mFeedback[delayLine] = feedback;

            float attenuationDB_HF = (-60.0f / (pProps->flDecayHFRatio * pProps->flDecayTime)) * this->mpSfxDsp->mLateDelayLenSec[delayLine];
            attenuationDB_HF -= attenuationDB;
            float attenuation_HF = FMOD_POW(10.0f, attenuationDB_HF / 20.0f);

            err |= this->Calculate1stOrderLowpassCoeff(attenuation_HF, pProps->flHFReference, static_cast<float>(this->mOutputRate), &K);
            this->mpSfxDsp->mDecayHF[delayLine] = 1.0f - K;
        }
    }

    this->SetReverbLevel(&this->mProps);
    return err;
}

bool DSPSfxReverb::SetDelayLineLengths(I3DL2_LISTENERPROPERTIES* pProps) {
    constexpr float MIN_DENS = 0.1f;
    constexpr float DENS_SCAL = 1.0f - MIN_DENS;

    float density = 0.01f * pProps->flDensity;
    density *= (density * density);

    density *= DENS_SCAL;
    density += MIN_DENS;

    if (density < MIN_DENS) {
        density = MIN_DENS;
    } else if (density > 1.0f) {
        density = 1.0f;
    }

    float lowestDelayLenSec = LOWEST_DELAY_LINE_LENGTH_SEC * density;

    this->mpSfxDsp->SetLateDelays(
        lowestDelayLenSec,
        DELAY_LINE_RATIO,
        LOWEST_DELAY_LINE_LENGTH_B_SEC,
        DELAY_LINE_RATIO_B,
        static_cast<float>(this->mOutputRate));

    bool err = SetDecayTime(pProps);
    return err;
}

bool DSPSfxReverb::SetDensity(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->flDensity < I3DL2LISTENER_MINDENSITY) {
        pProps->flDensity = I3DL2LISTENER_MINDENSITY;
    } else if (pProps->flDensity > I3DL2LISTENER_MAXDENSITY) {
        pProps->flDensity = I3DL2LISTENER_MAXDENSITY;
    }

    this->mProps.flDensity = pProps->flDensity;

    bool err = this->SetDelayLineLengths(pProps);
    return err;
}

bool DSPSfxReverb::SetDiffusion(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->flDiffusion < I3DL2LISTENER_MINDIFFUSION) {
        pProps->flDiffusion = I3DL2LISTENER_MINDIFFUSION;
    } else if (pProps->flDiffusion > I3DL2LISTENER_MAXDIFFUSION) {
        pProps->flDiffusion = I3DL2LISTENER_MAXDIFFUSION;
    }

    this->mProps.flDiffusion = pProps->flDiffusion;

    float diffusion = pProps->flDiffusion * 0.01f;
    diffusion = diffusion * (FMOD_PI / 4.0f);
    this->mpSfxDsp->mHadamard = FMOD_TAN(diffusion);

    float diffusionScale = FMOD_POW(FMOD_COS(diffusion), this->mpSfxDsp->mNumMatrixStages);
    this->mpSfxDsp->mDiffusionScale = diffusionScale;

    return false;
}

bool DSPSfxReverb::SetHFReference(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->flHFReference < I3DL2LISTENER_MINHFREFERENCE) {
        pProps->flHFReference = I3DL2LISTENER_MINHFREFERENCE;
    } else if (pProps->flHFReference > I3DL2LISTENER_MAXHFREFERENCE) {
        pProps->flHFReference = I3DL2LISTENER_MAXHFREFERENCE;
    }

    this->mProps.flHFReference = pProps->flHFReference;

    this->SetRoomHF(&this->mProps);
    this->SetDecayTime(&this->mProps);

    return false;
}

bool DSPSfxReverb::SetLFReference(float lfReference) {
    if (lfReference < FMODLISTENER_MINLFREFERENCE) {
        lfReference = FMODLISTENER_MINLFREFERENCE;
    } else if (lfReference > FMODLISTENER_MAXLFREFERENCE) {
        lfReference = FMODLISTENER_MAXLFREFERENCE;
    }

    this->mLFProps.mLFReference = lfReference;
    this->SetRoomLF(this->mLFProps.mRoomLF);

    return false;
}

bool DSPSfxReverb::SetReflectionsDelay(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->flReflectionsDelay < I3DL2LISTENER_MINREFLECTIONSDELAY) {
        pProps->flReflectionsDelay = I3DL2LISTENER_MINREFLECTIONSDELAY;
    } else if (pProps->flReflectionsDelay > I3DL2LISTENER_MAXREFLECTIONSDELAY) {
        pProps->flReflectionsDelay = I3DL2LISTENER_MAXREFLECTIONSDELAY;
    }

    this->mProps.flReflectionsDelay = pProps->flReflectionsDelay;
    this->mpSfxDsp->mEarlyLateSec[0] = this->mProps.flReflectionsDelay;

    auto refDelaySamples = static_cast<int32_t>(static_cast<float>(this->mOutputRate) * this->mProps.flReflectionsDelay);
    if (!refDelaySamples) {
        refDelaySamples++;
    }

    this->mpSfxDsp->mEarlyLateSamples[0] = refDelaySamples;
    this->SetReverbDelay(&this->mProps);
    return false;
}

bool DSPSfxReverb::SetReflectionsLevel(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->lReflections < I3DL2LISTENER_MINREFLECTIONS) {
        pProps->lReflections = I3DL2LISTENER_MINREFLECTIONS;
    } else if (pProps->lReflections > I3DL2LISTENER_MAXREFLECTIONS) {
        pProps->lReflections = I3DL2LISTENER_MAXREFLECTIONS;
    }

    this->mProps.lReflections = pProps->lReflections;
    float reflectionsLevelDB = static_cast<float>(pProps->lReflections + pProps->lRoom) * 0.01f;

    float ampGain = FMOD_POW(10.0f, reflectionsLevelDB / 20.0f);

    this->mpSfxDsp->mERgain = ampGain * FMOD_SQRT(1.0f / static_cast<float>(kNumEarlyDelayTaps + 1.0f));

    return false;
}

bool DSPSfxReverb::SetReverbDelay(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->flReverbDelay < I3DL2LISTENER_MINREVERBDELAY) {
        pProps->flReverbDelay = I3DL2LISTENER_MINREVERBDELAY;
    } else if (pProps->flReverbDelay > I3DL2LISTENER_MAXREVERBDELAY) {
        pProps->flReverbDelay = I3DL2LISTENER_MAXREVERBDELAY;
    }

    this->mProps.flReverbDelay = pProps->flReverbDelay;

    float refPlusRevDelaySec = this->mProps.flReflectionsDelay + this->mProps.flReverbDelay;
    this->mpSfxDsp->SetLate_EarlyLateDelayTaps(refPlusRevDelaySec, kEarlyLateNextLengthSec, kEarlyLateDelayRatio, static_cast<float>(this->mOutputRate));

    return false;
}

bool DSPSfxReverb::SetReverbLevel(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->lReverb < I3DL2LISTENER_MINREVERB) {
        pProps->lReverb = I3DL2LISTENER_MINREVERB;
    } else if (pProps->lReverb > I3DL2LISTENER_MAXREVERB) {
        pProps->lReverb = I3DL2LISTENER_MAXREVERB;
    }

    this->mProps.lReverb = pProps->lReverb;

    float reverbLevelDB = static_cast<float>(pProps->lReverb + pProps->lRoom) * 0.01f;
    float reverbGain = FMOD_POW(10.0f, reverbLevelDB / 20.0f);
    float powerGain;

    float aveFeedbackPowerGain = 0;

    for (int32_t delayLine = 0; delayLine < this->mpSfxDsp->mNumLateReverbDelays; delayLine++) {
        float feedbackGain = this->mpSfxDsp->mFeedback[delayLine];
        aveFeedbackPowerGain += (feedbackGain * feedbackGain);
    }

    aveFeedbackPowerGain /= this->mpSfxDsp->mNumLateReverbDelays;

    if (aveFeedbackPowerGain != 1.0f) {
        powerGain = 1.0f + (aveFeedbackPowerGain / (1.0f - aveFeedbackPowerGain));
    } else {
        powerGain = 1.0f;
    }

    float ampGain;

    if (powerGain > 0.0f) {
        ampGain = reverbGain / FMOD_SQRT(powerGain);
    } else {
        ampGain = reverbGain;
    }

    this->mpSfxDsp->mLRgain = ampGain * FMOD_SQRT(1.0 / static_cast<float>(kNumLateReverbDelays));

    return false;
}

bool DSPSfxReverb::SetRoom(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->lRoom < I3DL2LISTENER_MINROOM) {
        pProps->lRoom = I3DL2LISTENER_MINROOM;
    } else if (pProps->lRoom > I3DL2LISTENER_MAXROOM) {
        pProps->lRoom = I3DL2LISTENER_MAXROOM;
    }

    this->mProps.lRoom = pProps->lRoom;
    bool err = this->SetReflectionsLevel(pProps);
    err |= this->SetReverbLevel(pProps);

    return err;
}

bool DSPSfxReverb::SetRoomHF(I3DL2_LISTENERPROPERTIES* pProps) {
    if (pProps->lRoomHF < I3DL2LISTENER_MINROOMHF) {
        pProps->lRoomHF = I3DL2LISTENER_MINROOMHF;
    } else if (pProps->lRoomHF > I3DL2LISTENER_MAXROOMHF) {
        pProps->lRoomHF = I3DL2LISTENER_MAXROOMHF;
    }

    this->mProps.lRoomHF = pProps->lRoomHF;

    float gainDB = static_cast<float>(pProps->lRoomHF) * 0.01f / 2.0f;
    float gain = FMOD_POW(10.0f, gainDB / 20.0f);

    float K;
    bool err = this->Calculate1stOrderLowpassCoeff(gain, pProps->flHFReference, static_cast<float>(this->mOutputRate), &K);

    this->mpSfxDsp->mRoomHF = 1.0f - K;

    return err;
}

bool DSPSfxReverb::SetRoomLF(int32_t roomLF) {
    if (roomLF < FMODLISTENER_MINROOMLF) {
        roomLF = FMODLISTENER_MINROOMLF;
    } else if (roomLF > FMODLISTENER_MAXROOMLF) {
        roomLF = FMODLISTENER_MAXROOMLF;
    }

    this->mLFProps.mRoomLF = roomLF;
    this->mpSfxDsp->mRoomLF = roomLF / 100.0f;

    bool err = this->CalculateShelfCoeffs(
        this->mpSfxDsp->mRoomLF,
        this->mLFProps.mLFReference,
        static_cast<float>(this->mOutputRate),
        &this->mpSfxDsp->mRoomLFcoeffs.a0,
        &this->mpSfxDsp->mRoomLFcoeffs.a1,
        &this->mpSfxDsp->mRoomLFcoeffs.a2,
        &this->mpSfxDsp->mRoomLFcoeffs.b1,
        &this->mpSfxDsp->mRoomLFcoeffs.b2);

    return err;
}

FMOD_RESULT DSPSfxReverb::createInternal() {
    FMOD_RESULT result;
    I3DL2_LISTENERPROPERTIES props = {
        -600, 0, 0.0f, 20.0f, 0.1f, 1000, 0.3f, 0, 0.1f, 100.0f, 100.0f, 5000.0f
    };

    Plugin::init();

    this->mSystem->getSoftwareFormat(&this->mOutputRate, nullptr, nullptr, nullptr, nullptr, nullptr);

    this->mDryLevelLin = 0.0f;
    this->mDryLevelmB = -100000.0f;

    this->mLFProps.mRoomLF = FMODLISTENER_DEFAULTROOMLF;
    this->mLFProps.mLFReference = FMODLISTENER_DEFAULTLFREFERENCE;

    this->mUpdateFlag = false;

    FMOD_memcpy(&this->mProps, &props, sizeof(I3DL2_LISTENERPROPERTIES));

    this->mpSfxDsp = static_cast<ASfxDsp*>(FMOD_Memory_Alloc(sizeof(ASfxDsp)));
    if (!this->mpSfxDsp) {
        return FMOD_ERR_MEMORY;
    }

    if (this->mpSfxDsp->init(static_cast<float>(this->mOutputRate))) {
        return FMOD_ERR_MEMORY;
    }

    this->mpSfxDsp->UpdateBufferSize(this->mSystem->mDSPBlockSize);
    this->mpSfxDsp->mNumLateReverbDelays = kNumLateReverbDelays;
    this->mpSfxDsp->mNumMatrixStages = static_cast<int32_t>(FMOD_LOG(static_cast<float>(this->mpSfxDsp->mNumLateReverbDelays)) / FMOD_LOG(2.0f) + 0.5f);
    this->mpSfxDsp->ClearBuffers();

    for (int32_t i = 0; i < this->mDescription.numparameters; i++) {
        result = this->setParameter(i, this->mDescription.paramdesc[i].defaultval);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mLFProps.mLFReference = this->mUpdateLFProps.mLFReference;
    FMOD_memcpy(&this->mProps, &this->mUpdateProps, sizeof(I3DL2_LISTENERPROPERTIES));
    this->mLFProps.mRoomLF = this->mUpdateLFProps.mRoomLF;
    this->mUpdateFlag = false;

    this->SetRoom(&this->mProps);
    this->SetRoomHF(&this->mProps);
    this->SetDecayTime(&this->mProps);
    this->SetDecayHFRatio(&this->mProps);
    this->SetReflectionsLevel(&this->mProps);
    this->SetReflectionsDelay(&this->mProps);
    this->SetReverbLevel(&this->mProps);
    this->SetReverbDelay(&this->mProps);
    this->SetDiffusion(&this->mProps);
    this->SetDensity(&this->mProps);
    this->SetHFReference(&this->mProps);
    this->SetRoomLF(this->mLFProps.mRoomLF);
    this->SetLFReference(this->mLFProps.mLFReference);

    return FMOD_OK;
}

FMOD_RESULT DSPSfxReverb::releaseInternal() {
    this->mpSfxDsp->close();

    FMOD_Memory_Free(this->mpSfxDsp);
    this->mpSfxDsp = nullptr;

    return FMOD_OK;
}

FMOD_RESULT DSPSfxReverb::resetInternal() {
    this->mpSfxDsp->UpdateBufferSize(1024);
    this->mpSfxDsp->ClearBuffers();

    return FMOD_OK;
}

FMOD_RESULT DSPSfxReverb::readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    if (this->mUpdateFlag) {
        if (this->mProps.lRoom != this->mUpdateProps.lRoom) {
            this->mProps.lRoom = this->mUpdateProps.lRoom;
            this->SetRoom(&this->mProps);
        }

        if (this->mProps.lRoomHF != this->mUpdateProps.lRoomHF) {
            this->mProps.lRoomHF = this->mUpdateProps.lRoomHF;
            this->SetRoomHF(&this->mProps);
        }

        if (this->mProps.flRoomRolloffFactor != this->mUpdateProps.flRoomRolloffFactor) {
            this->mProps.flRoomRolloffFactor = this->mUpdateProps.flRoomRolloffFactor;
        }

        if (this->mProps.flDecayTime != this->mUpdateProps.flDecayTime) {
            this->mProps.flDecayTime = this->mUpdateProps.flDecayTime;
            this->SetDecayTime(&this->mProps);
        }

        if (this->mProps.flDecayHFRatio != this->mUpdateProps.flDecayHFRatio) {
            this->mProps.flDecayHFRatio = this->mUpdateProps.flDecayHFRatio;
            this->SetDecayHFRatio(&this->mProps);
        }

        if (this->mProps.lReflections != this->mUpdateProps.lReflections) {
            this->mProps.lReflections = this->mUpdateProps.lReflections;
            this->SetReflectionsLevel(&this->mProps);
        }

        if (this->mProps.flReflectionsDelay != this->mUpdateProps.flReflectionsDelay) {
            this->mProps.flReflectionsDelay = this->mUpdateProps.flReflectionsDelay;
            this->SetReflectionsDelay(&this->mProps);
        }

        if (this->mProps.lReverb != this->mUpdateProps.lReverb) {
            this->mProps.lReverb = this->mUpdateProps.lReverb;
            this->SetReverbLevel(&this->mProps);
        }

        if (this->mProps.flReverbDelay != this->mUpdateProps.flReverbDelay) {
            this->mProps.flReverbDelay = this->mUpdateProps.flReverbDelay;
            this->SetReverbDelay(&this->mProps);
        }

        if (this->mProps.flDiffusion != this->mUpdateProps.flDiffusion) {
            this->mProps.flDiffusion = this->mUpdateProps.flDiffusion;
            this->SetDiffusion(&this->mProps);
        }

        if (this->mProps.flDensity != this->mUpdateProps.flDensity) {
            this->mProps.flDensity = this->mUpdateProps.flDensity;
            this->SetDensity(&mProps);
        }

        if (this->mProps.flHFReference != this->mUpdateProps.flHFReference) {
            this->mProps.flHFReference = this->mUpdateProps.flHFReference;
            this->SetHFReference(&mProps);
        }

        if (this->mLFProps.mRoomLF != this->mUpdateLFProps.mRoomLF) {
            this->mLFProps.mRoomLF = this->mUpdateLFProps.mRoomLF;
            this->SetRoomLF(this->mUpdateLFProps.mRoomLF);
        }

        if (this->mUpdateLFProps.mLFReference != this->mLFProps.mLFReference) {
            this->mLFProps.mLFReference = this->mUpdateLFProps.mLFReference;
            this->SetLFReference(this->mUpdateLFProps.mLFReference);
        }

        this->mUpdateFlag = false;
    }

    this->mpSfxDsp->DoDSPProcessing(
        inbuffer,
        outbuffer,
        inchannels,
        length,
        static_cast<float>(this->mOutputRate),
        this->mDryLevelLin);

    return FMOD_OK;
}

FMOD_RESULT DSPSfxReverb::setParameterInternal(int32_t index, float value) {
    FMOD_RESULT result = FMOD_OK;

    switch (index) {
    case FMOD_DSP_SFXREVERB_DRYLEVEL: {
        this->mDryLevelmB = value;
        this->mDryLevelLin = FMOD_POW(10.0f, this->mDryLevelmB / 2000.0f);
        break;
    }
    case FMOD_DSP_SFXREVERB_ROOM: {
        this->mUpdateProps.lRoom = static_cast<int32_t>(value + 0.5f);
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_ROOMHF: {
        this->mUpdateProps.lRoomHF = static_cast<int32_t>(value + 0.5f);
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_ROOMROLLOFFFACTOR: {
        this->mUpdateProps.flRoomRolloffFactor = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_DECAYTIME: {
        this->mUpdateProps.flDecayTime = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_DECAYHFRATIO: {
        this->mUpdateProps.flDecayHFRatio = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_REFLECTIONSLEVEL: {
        this->mUpdateProps.lReflections = static_cast<int32_t>(value + 0.5f);
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_REFLECTIONSDELAY: {
        this->mUpdateProps.flReflectionsDelay = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_REVERBLEVEL: {
        this->mUpdateProps.lReverb = static_cast<int32_t>(value + 0.5f);
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_REVERBDELAY: {
        this->mUpdateProps.flReverbDelay = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_DIFFUSION: {
        this->mUpdateProps.flDiffusion = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_DENSITY: {
        this->mUpdateProps.flDensity = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_HFREFERENCE: {
        this->mUpdateProps.flHFReference = value;
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_ROOMLF: {
        this->mUpdateLFProps.mRoomLF = static_cast<int32_t>(value + 0.5f);
        this->mUpdateFlag = true;
        break;
    }
    case FMOD_DSP_SFXREVERB_LFREFERENCE: {
        this->mUpdateLFProps.mLFReference = value;
        this->mUpdateFlag = true;
        break;
    }
    default: {
        result = FMOD_ERR_INVALID_PARAM;
        break;
    }
    }

    return result;
}

FMOD_RESULT DSPSfxReverb::getParameterInternal(int32_t index, float* value, char* valuestr) {
    switch (index) {
    case FMOD_DSP_SFXREVERB_DRYLEVEL:
        if (value) {
            *value = this->mDryLevelmB;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mDryLevelmB);
        }
        break;
    case FMOD_DSP_SFXREVERB_ROOM:
        if (value) {
            *value = static_cast<float>(this->mProps.lRoom);
        }
        if (valuestr) {
            sprintf(valuestr, "%d", this->mProps.lRoom);
        }
        break;
    case FMOD_DSP_SFXREVERB_ROOMHF:
        if (value) {
            *value = static_cast<float>(this->mProps.lRoomHF);
        }
        if (valuestr) {
            sprintf(valuestr, "%d", this->mProps.lRoomHF);
        }
        break;
    case FMOD_DSP_SFXREVERB_ROOMROLLOFFFACTOR:
        if (value) {
            *value = this->mProps.flRoomRolloffFactor;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flRoomRolloffFactor);
        }
        break;
    case FMOD_DSP_SFXREVERB_DECAYTIME:
        if (value) {
            *value = this->mProps.flDecayTime;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flDecayTime);
        }
        break;
    case FMOD_DSP_SFXREVERB_DECAYHFRATIO:
        if (value) {
            *value = this->mProps.flDecayHFRatio;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flDecayHFRatio);
        }
        break;
    case FMOD_DSP_SFXREVERB_REFLECTIONSLEVEL:
        if (value) {
            *value = static_cast<float>(this->mProps.lReflections);
        }
        if (valuestr) {
            sprintf(valuestr, "%d", this->mProps.lReflections);
        }
        break;
    case FMOD_DSP_SFXREVERB_REFLECTIONSDELAY:
        if (value) {
            *value = this->mProps.flReflectionsDelay;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flReflectionsDelay);
        }
        break;
    case FMOD_DSP_SFXREVERB_REVERBLEVEL:
        if (value) {
            *value = static_cast<float>(this->mProps.lReverb);
        }
        if (valuestr) {
            sprintf(valuestr, "%d", this->mProps.lReverb);
        }
        break;
    case FMOD_DSP_SFXREVERB_REVERBDELAY:
        if (value) {
            *value = this->mProps.flReverbDelay;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flReverbDelay);
        }
        break;
    case FMOD_DSP_SFXREVERB_DIFFUSION:
        if (value) {
            *value = this->mProps.flDiffusion;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flDiffusion);
        }
        break;
    case FMOD_DSP_SFXREVERB_DENSITY:
        if (value) {
            *value = this->mProps.flDensity;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flDensity);
        }
        break;
    case FMOD_DSP_SFXREVERB_HFREFERENCE:
        if (value) {
            *value = this->mProps.flHFReference;
        }
        if (valuestr) {
            sprintf(valuestr, "%f", this->mProps.flHFReference);
        }
        break;
    case FMOD_DSP_SFXREVERB_ROOMLF:
        if (value) {
            *value = static_cast<float>(this->mLFProps.mRoomLF);
        }
        if (valuestr) {
            sprintf(valuestr, "%d", this->mLFProps.mRoomLF);
        }
        break;
    case FMOD_DSP_SFXREVERB_LFREFERENCE:
        if (value) {
            *value = this->mLFProps.mLFReference;
        }
        if (valuestr) {
            sprintf(valuestr, "%d", static_cast<int32_t>(this->mLFProps.mLFReference));
            return FMOD_OK;
        }
        break;
    default:
        return FMOD_ERR_INVALID_PARAM;
    }
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK DSPSfxReverb::createCallback(FMOD_DSP_STATE* dsp) {
    auto sfxreverb = static_cast<DSPSfxReverb*>(dsp);
    return sfxreverb->createInternal();
}

FMOD_RESULT F_CALLBACK DSPSfxReverb::releaseCallback(FMOD_DSP_STATE* dsp) {
    auto sfxreverb = static_cast<DSPSfxReverb*>(dsp);
    return sfxreverb->releaseInternal();
}

FMOD_RESULT F_CALLBACK DSPSfxReverb::resetCallback(FMOD_DSP_STATE* dsp) {
    auto sfxreverb = static_cast<DSPSfxReverb*>(dsp);
    return sfxreverb->resetInternal();
}

FMOD_RESULT F_CALLBACK DSPSfxReverb::readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels) {
    auto sfxreverb = static_cast<DSPSfxReverb*>(dsp);
    return sfxreverb->readInternal(inbuffer, outbuffer, length, inchannels, outchannels);
}

FMOD_RESULT F_CALLBACK DSPSfxReverb::setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value) {
    auto sfxreverb = static_cast<DSPSfxReverb*>(dsp);
    return sfxreverb->setParameterInternal(index, value);
}

FMOD_RESULT F_CALLBACK DSPSfxReverb::getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr) {
    auto sfxreverb = static_cast<DSPSfxReverb*>(dsp);
    return sfxreverb->getParameterInternal(index, value, valuestr);
}

FMOD_DSP_DESCRIPTION_EX* DSPSfxReverb::getDescriptionEx() {
    FMOD_memset(&dspsfxreverb, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));

    FMOD_strcpy(dspsfxreverb.name, "SFX Reverb");
    dspsfxreverb.version = 0x10100;

    dspsfxreverb.create = DSPSfxReverb::createCallback;
    dspsfxreverb.release = DSPSfxReverb::releaseCallback;
    dspsfxreverb.reset = DSPSfxReverb::resetCallback;

    dspsfxreverb.read = DSPSfxReverb::readCallback;

    dspsfxreverb.numparameters = (sizeof(dspsfxreverb_param) / sizeof(dspsfxreverb_param[0]));
    dspsfxreverb.paramdesc = dspsfxreverb_param;
    dspsfxreverb.setparameter = DSPSfxReverb::setParameterCallback;
    dspsfxreverb.getparameter = DSPSfxReverb::getParameterCallback;

    dspsfxreverb.mType = FMOD_DSP_TYPE_SFXREVERB;
    dspsfxreverb.mCategory = FMOD_DSP_CATEGORY_FILTER;
    dspsfxreverb.mSize = sizeof(DSPSfxReverb);

    return &dspsfxreverb;
}

} // namespace FMOD
