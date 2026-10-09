#ifndef XMODEX_DSP_DSP_CONNECTION_HPP
#define XMODEX_DSP_DSP_CONNECTION_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"

#define DSP_MAXLEVELS_MAX 16
#define DSP_MAXLEVELS_OUT 16

#define DSP_LEVEL_SMALLVAL 0.0001f

namespace FMOD {

constexpr int32_t DSP_RAMPCOUNT = 64;

class DSPI;

class DSPConnection {
    public:
        LinkedListNode mInputNode;
        LinkedListNode mOutputNode;
        LinkedListNode* mNode;
        int16_t mMaxOutputLevels;
        int16_t mMaxInputLevels;
        float* mLevel[DSP_MAXLEVELS_OUT];
        float* mLevelCurrent[DSP_MAXLEVELS_OUT];
        float* mLevelDelta[DSP_MAXLEVELS_OUT];
        DSPI* mInputUnit;
        DSPI* mOutputUnit;
        int16_t mRampCount;
        int16_t mSetLevelsUsed;
        float mVolume;

    public:
        FMOD_RESULT init(float*& levelmemory, int32_t maxoutputlevels, int32_t maxinputlevels);
        FMOD_RESULT reset();
        FMOD_RESULT setUnity();
        FMOD_RESULT mixAndRamp(float* outbuffer, float* inbuffer, int32_t outchannels, int32_t inchannels, uint32_t length);
        FMOD_RESULT rampTo();
        FMOD_RESULT checkUnity(int32_t outchannels, int32_t inchannels);
        FMOD_RESULT setPan(float pan, int32_t outchannels, int32_t inchannels, FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT setMix(float volume);
        FMOD_RESULT setLevels(float* levels, int32_t numinputlevels);
        FMOD_RESULT getLevels(float* levels, int32_t numinputlevels);
        FMOD_RESULT copy(DSPConnection* source);
        FMOD_RESULT mix(float* outbuffer, float* inbuffer, int32_t outchannels, int32_t inchannels, uint32_t length);
};

} // namespace FMOD

#endif
