#ifndef XMODEX_DSP_DSP_RESAMPLER_HPP
#define XMODEX_DSP_DSP_RESAMPLER_HPP

#include "xmodex/Types.hpp"
#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

enum FMOD_RESAMPLER_END {
    FMOD_RESAMPLER_END_MIXBUFFER,
    FMOD_RESAMPLER_END_RESAMPLEBUFFER,
    FMOD_RESAMPLER_END_SOUND,
    FMOD_RESAMPLER_END_SUBSOUND
};

constexpr int32_t FMOD_DSP_RESAMPLER_OVERFLOWLENGTH = 4;

class DSPResampler : public DSPFilter {
    public:
        FMOD_UINT64P mPosition;
        FMOD_SINT64P mSpeed;
        float mFrequency;
        int32_t mTargetFrequency;
        FMOD_UINT64P mResamplePosition;
        void* mResampleBufferMemory;
        void* mResampleBuffer;
        int32_t mResampleBufferChannels;
        uint32_t mResampleBlockLength;
        uint32_t mResampleBufferLength;
        uint32_t mResampleBufferPos;
        uint32_t mResampleFinishPos;
        uint32_t mOverflowLength;
        uint32_t mReadPosition;
        int32_t mFill;
        uint32_t mLength;
        uint32_t mLoopStart;
        int32_t mLoopLength;
        uint32_t mLoopCount;
        FMOD_MODE mMode;

    public:
        DSPResampler();
        FMOD_RESULT release(bool freethis);
        FMOD_RESULT setFrequency(float frequency);
        FMOD_RESULT setPosition(uint32_t position, bool processinputs);
        FMOD_RESULT alloc(FMOD_DSP_DESCRIPTION_EX* description);
        FMOD_RESULT execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode);
};

} // namespace FMOD

#endif
