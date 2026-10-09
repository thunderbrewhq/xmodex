#ifndef XMODEX_DSP_DSP_CODEC_ADPCM_HPP
#define XMODEX_DSP_DSP_CODEC_ADPCM_HPP

#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/dsp/DSPCodec.hpp"

namespace FMOD {

class DSPCodecADPCM : public DSPCodec {
    public:
        char mResampleBufferMemory[656];
        CodecWav mCodecMemory;
};

} // namespace FMOD

#endif
