#ifndef XMODEX_OS_SIMD_HPP
#define XMODEX_OS_SIMD_HPP

#include <cstdint>

#define FMOD_OS_SupportsSIMD() (FMOD_OS_SupportsSSE() ? true : false)

int32_t FMOD_OS_SupportsSSE();

#endif
