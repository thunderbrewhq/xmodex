#ifndef XMODEX_CODEC_CODEC_TAG_HPP
#define XMODEX_CODEC_CODEC_TAG_HPP

#include "xmodex/codec/Codec.hpp"

namespace FMOD {

const FMOD_SOUND_TYPE FMOD_SOUND_TYPE_TAG = static_cast<FMOD_SOUND_TYPE>(1000);

class CodecTag : public Codec {
    private:
        FMOD_RESULT readID3v1();
        FMOD_RESULT readID3v2();
        FMOD_RESULT readID3v2FromFooter();
        FMOD_RESULT readASF();
        FMOD_RESULT readTags();
        FMOD_RESULT openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        FMOD_RESULT closeInternal();

    public:
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        static FMOD_RESULT F_CALLBACK setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);
        static FMOD_RESULT F_CALLBACK openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        static FMOD_CODEC_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
