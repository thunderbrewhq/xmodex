#ifndef XMODEX_CODEC_IMAA_DPCM_HPP
#define XMODEX_CODEC_IMAA_DPCM_HPP

#include "fmod.hpp"

namespace FMOD {

FMOD_RESULT IMAAdpcm_DecodeM16(
    uint8_t* pbSrc,
    int16_t* pbDst,
    uint32_t cBlocks,
    uint32_t nBlockAlignment,
    uint32_t cSamplesPerBlock,
    int32_t channels);

FMOD_RESULT IMAAdpcm_DecodeS16(
    uint8_t* pbSrc,
    int16_t* pbDst,
    uint32_t cBlocks,
    uint32_t nBlockAlignment,
    uint32_t cSamplesPerBlock);

} // namespace FMOD

#endif
