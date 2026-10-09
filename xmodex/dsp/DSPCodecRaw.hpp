#ifndef XMODEX_DSP_DSP_CODEC_RAW_HPP
#define XMODEX_DSP_DSP_CODEC_RAW_HPP

#include "xmodex/codec/CodecRaw.hpp"
#include "xmodex/dsp/DSPCodec.hpp"

namespace FMOD {

class DSPCodecRaw : public DSPCodec {
    public:
        char mResampleBufferMemory[8720];
        CodecRaw mCodecMemory;
};

} // namespace FMOD

#endif
