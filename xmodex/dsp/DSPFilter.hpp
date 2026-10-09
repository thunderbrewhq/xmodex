#ifndef XMODEX_DSP_DSP_FILTER_HPP
#define XMODEX_DSP_DSP_FILTER_HPP

#include "xmodex/dsp/DSPI.hpp"

namespace FMOD {

class DSPFilter : public DSPI {
    private:
        float* mHistoryBuffer;
        uint32_t mHistoryPosition;
        int32_t mHistoryLength;
        int32_t mBufferChannels;

    public:
        DSPFilter();

        FMOD_RESULT execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT startBuffering(uint32_t historylength);
        FMOD_RESULT stopBuffering();
        FMOD_RESULT getHistoryBuffer(float** buffer, uint32_t* position, uint32_t* length);
        FMOD_RESULT release(bool freethis);
};

} // namespace FMOD

#endif
