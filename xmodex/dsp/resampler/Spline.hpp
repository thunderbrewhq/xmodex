#ifndef XMODEX_DSP_RESAMPLER_SPLINE_HPP
#define XMODEX_DSP_RESAMPLER_SPLINE_HPP

#include "fmod.hpp"
#include "xmodex/Types.hpp"

void FMOD_Resampler_Spline(
    float* out,
    int32_t outlength,
    void* src,
    FMOD_SOUND_FORMAT srcformat,
    FMOD_UINT64P* position,
    FMOD_SINT64P* speed,
    int32_t channels);

#endif
