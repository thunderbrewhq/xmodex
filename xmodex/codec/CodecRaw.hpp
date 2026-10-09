#ifndef XMODEX_CODEC_CODEC_RAW_HPP
#define XMODEX_CODEC_CODEC_RAW_HPP

#include "xmodex/codec/Codec.hpp"

namespace FMOD {

class CodecRaw : public Codec {
    public:
        FMOD_CODEC_WAVEFORMAT mWaveFormat;
        int32_t mSamplesPerADPCMBlock;

    public:
        FMOD_RESULT closeInternal();
        FMOD_RESULT canPointInternal();
        FMOD_RESULT openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        FMOD_RESULT readInternal(void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        FMOD_RESULT setPositionInternal(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);

        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK canPointCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        static FMOD_RESULT F_CALLBACK setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);
        static FMOD_CODEC_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
