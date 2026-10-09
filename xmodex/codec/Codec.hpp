#ifndef XMODEX_CODEC_CODEC_HPP
#define XMODEX_CODEC_CODEC_HPP

#include "xmodex/Metadata.hpp"
#include "xmodex/Plugin.hpp"
#include "xmodex/os/Library.hpp"

#define FMOD_CODEC_ACCURATELENGTH 0x00000001
#define FMOD_CODEC_FROMFSB 0x00000002
#define FMOD_CODEC_SEEKING 0x00000004
#define FMOD_CODEC_PADDED 0x00000008
#define FMOD_CODEC_USERLENGTH 0x00000010
#define FMOD_CODEC_HARDWAREMUSICVOICES 0x00000020
#define FMOD_CODEC_FSBXMAMIXEDCHANNELS 0x00000040
#define FMOD_CODEC_PADDED4 0x00000080

namespace FMOD {

class File;

typedef uint32_t FMOD_CODEC_FLAG;

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_RESETCALLBACK)(FMOD_CODEC_STATE* codec);
typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_CANPOINTCALLBACK)(FMOD_CODEC_STATE* codec);

struct FMOD_CODEC_DESCRIPTION_EX : public FMOD_CODEC_DESCRIPTION, public LinkedListNode {
        FMOD_SOUND_TYPE mType;
        int32_t mSize;
        FMOD_OS_LIBRARY* mModule;
        FMOD_CODEC_RESETCALLBACK reset;
        FMOD_CODEC_CANPOINTCALLBACK canpoint;
};

class Codec : public Plugin, public FMOD_CODEC_STATE {
    public:
        FMOD_CODEC_WAVEFORMAT* mWaveFormatMemory;
        FMOD_SOUND_TYPE mType;
        FMOD_CODEC_DESCRIPTION_EX mDescription;
        uint32_t mSrcDataOffset;
        uint32_t mLoopPoints[2];
        int32_t mSubSoundIndex;
        FMOD_CODEC_FLAG mFlags;
        uint32_t mBlockAlign;
        uint8_t* mReadBuffer;
        uint32_t mReadBufferLength;
        uint8_t* mPCMBuffer;
        uint8_t* mPCMBufferMemory;
        uint32_t mPCMBufferLength;
        uint32_t mPCMBufferLengthBytes;
        uint32_t mPCMBufferOffsetBytes;
        uint32_t mPCMBufferFilledBytes;
        FMOD_MODE mMode;
        FMOD_MODE mOriginalMode;
        Metadata* mMetadata;
        File* mFile;

    public:
        Codec();

        FMOD_RESULT init(FMOD_SOUND_TYPE type);
        bool readyForRelease();
        FMOD_RESULT release();

        FMOD_RESULT canPointTo();
        FMOD_RESULT getLength(uint32_t* length, FMOD_TIMEUNIT lengthtype);
        FMOD_RESULT getMetadataFromFile();
        FMOD_RESULT getPosition(uint32_t* position, FMOD_TIMEUNIT postype);
        FMOD_RESULT metaData(FMOD_TAGTYPE type, const char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype, bool unique);
        FMOD_RESULT read(void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        FMOD_RESULT reset();
        FMOD_RESULT setPosition(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);

        static FMOD_RESULT defaultFileRead(void* handle, void* buffer, uint32_t size, uint32_t* read, void* userdata);
        static FMOD_RESULT defaultFileSeek(void* handle, uint32_t pos, void* userdata);
        static FMOD_RESULT defaultGetWaveFormat(FMOD_CODEC_STATE* codec, int32_t index, FMOD_CODEC_WAVEFORMAT* waveformat_out);
        static FMOD_RESULT defaultMetaData(FMOD_CODEC_STATE* codec, FMOD_TAGTYPE type, char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype, int32_t unique);
};

} // namespace FMOD

#endif
