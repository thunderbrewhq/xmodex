#ifndef XMODEX_DSP_DSP_RESAMPLER_MULTI_INPUT_HPP
#define XMODEX_DSP_DSP_RESAMPLER_MULTI_INPUT_HPP

#include "xmodex/dsp/DSPResampler.hpp"

namespace FMOD {

class DSPResamplerMultiInput : public DSPResampler {
    public:
        DSPResamplerMultiInput();

        FMOD_RESULT execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT addInput(DSPI* target);
};

} // namespace FMOD

#endif
