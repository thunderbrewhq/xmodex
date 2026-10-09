#ifndef XMODEX_DSP_DSP_CODEC_HPP
#define XMODEX_DSP_DSP_CODEC_HPP

#include "fmod.hpp"
#include "xmodex/dsp/DSPResampler.hpp"
#include "xmodex/file/MemoryFile.hpp"

namespace FMOD {

class MemoryFile;
class DSPCodecPool;
class Codec;

class DSPCodec : public DSPResampler {
    public:
        int32_t mPoolIndex;
        MemoryFile mMemoryFile;
        uint32_t mPosition;
        uint32_t mNewPosition;
        DSPCodecPool* mPool;
        FMOD_CODEC_WAVEFORMAT mWaveFormat;
        Codec* mCodec;

    public:
        FMOD_RESULT createInternal();
        FMOD_RESULT getParameterInternal(int32_t index, float* value, char* valuestr);
        FMOD_RESULT readInternal(int16_t* inbuffer, int16_t* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT releaseInternal();
        FMOD_RESULT resetInternal();
        FMOD_RESULT setParameterInternal(int32_t index, float value);
        FMOD_RESULT setPositionInternal(uint32_t position);
        FMOD_RESULT release(bool freethis);

        static FMOD_RESULT F_CALLBACK createCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK releaseCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK resetCallback(FMOD_DSP_STATE* dsp);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_DSP_STATE* dsp, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK setParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float value);
        static FMOD_RESULT F_CALLBACK getParameterCallback(FMOD_DSP_STATE* dsp, int32_t index, float* value, char* valuestr);
        static FMOD_RESULT F_CALLBACK setPositionCallback(FMOD_DSP_STATE* dsp, uint32_t pos);
        static FMOD_DSP_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
