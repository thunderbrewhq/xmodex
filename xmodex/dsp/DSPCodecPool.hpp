#ifndef XMODEX_DSP_DSP_CODEC_POOL_HPP
#define XMODEX_DSP_DSP_CODEC_POOL_HPP

#include "fmod.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include <cstdint>

#define FMOD_DSP_CODECPOOL_MAXCODECS 256

namespace FMOD {

class SystemI;
class DSPCodec;

class DSPCodecPool {
    public:
        SystemI* mSystem;
        int32_t mNumDSPCodecs;
        DSPCodec** mPool;
        bool mAllocated[FMOD_DSP_CODECPOOL_MAXCODECS];
        uint8_t* mReadBuffer;

    public:
        FMOD_RESULT close();
        FMOD_RESULT alloc(DSPI** dsp);
        FMOD_RESULT areAnyFree();
        FMOD_RESULT init(FMOD_DSP_CATEGORY category, int32_t resamplerpcmblocksize, int32_t numdspcodecs);
};

} // namespace FMOD

#endif
