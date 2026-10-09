#ifndef XMODEX_DSP_DSP_CODEC_MPEG
#define XMODEX_DSP_DSP_CODEC_MPEG

#include "xmodex/codec/CodecMPEG.hpp"
#include "xmodex/dsp/DSPCodec.hpp"

namespace FMOD {

class DSPCodecMPEG : public DSPCodec {
    public:
        char mResampleBufferMemory[9360];
        CodecMPEG mCodecMemory;
        CodecMPEG_MemoryBlock mCodecMemoryBlock;
};

} // namespace FMOD

#endif
