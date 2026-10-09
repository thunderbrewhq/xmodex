#ifndef XMODEX_DSP_DSP_SFX_REVERB_HPP
#define XMODEX_DSP_DSP_SFX_REVERB_HPP

#include "xmodex/dsp/3DL2.hpp"
#include "xmodex/dsp/DSPFilter.hpp"

class ASfxDsp;

namespace FMOD {

struct SFX_REVERB_LFPROPS {
        int32_t mRoomLF;
        float mLFReference;
};

class DSPSfxReverb : public DSPFilter {
    private:
        I3DL2_LISTENERPROPERTIES mProps;
        I3DL2_LISTENERPROPERTIES mUpdateProps;
        bool mUpdateFlag;
        float mDryLevelmB;
        float mDryLevelLin;
        SFX_REVERB_LFPROPS mLFProps;
        SFX_REVERB_LFPROPS mUpdateLFProps;
        ASfxDsp* mpSfxDsp;
        int32_t mOutputRate;

    private:
        bool Calculate1stOrderLowpassCoeff(float gain, float cutoff, float sampleRate, float* pK);
        bool CalculateShelfCoeffs(float gain, float cutoff, float sampleRate, float* a0, float* a1, float* a2, float* b1, float* b2);
        bool SetDecayHFRatio(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetDecayTime(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetDelayLineLengths(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetDensity(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetDiffusion(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetHFReference(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetLFReference(float lfReference);
        bool SetReflectionsDelay(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetReflectionsLevel(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetReverbDelay(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetReverbLevel(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetRoom(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetRoomHF(I3DL2_LISTENERPROPERTIES* pProps);
        bool SetRoomLF(int32_t roomLF);

    private:
        FMOD_RESULT createInternal();
        FMOD_RESULT releaseInternal();
        FMOD_RESULT resetInternal();
        FMOD_RESULT readInternal(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);

    public:
        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK releaseCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK resetCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
