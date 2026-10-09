#ifndef XMODEX_CODEC_CODEC_OGG_VORBIS_HPP
#define XMODEX_CODEC_CODEC_OGG_VORBIS_HPP

#include "xmodex/SyncPoint.hpp"
#include "xmodex/codec/Codec.hpp"
#include <vorbis/vorbisfile.h>

namespace FMOD {

class CodecOggVorbis : public Codec {
    private:
        OggVorbis_File mVf;
        SyncPoint* mSyncPoint;
        uint32_t mNumSyncPoints;
        FMOD_CODEC_WAVEFORMAT mWaveFormat;

    public:
        FMOD_RESULT closeInternal();
        FMOD_RESULT openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        FMOD_RESULT readInternal(void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        FMOD_RESULT readVorbisComments();
        FMOD_RESULT setPositionInternal(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);

        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        static FMOD_RESULT F_CALLBACK setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);
        static FMOD_CODEC_DESCRIPTION_EX* getDescriptionEx();

    public:
        static bool gInitialized;
};

} // namespace FMOD

#endif
