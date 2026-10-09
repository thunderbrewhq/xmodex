#ifndef XMODEX_DSP_RESAMPLER_NO_INTERP_HPP
#define XMODEX_DSP_RESAMPLER_NO_INTERP_HPP

#include "fmod.hpp"
#include "xmodex/Types.hpp"

void FMOD_Resampler_NoInterp(float* out, int32_t outlength, void* src, FMOD_SOUND_FORMAT srcformat, FMOD_UINT64P* position, FMOD_SINT64P* speed, int32_t channels);

#endif
