#ifndef XMODEX_CODEC_CODEC_WAV_HPP
#define XMODEX_CODEC_CODEC_WAV_HPP

#include "xmodex/codec/Codec.hpp"

#define WAVE_FORMAT_EXTENSIBLE 0xFFFE
#define WAVE_FORMAT_IEEE_FLOAT 3
#define WAVE_FORMAT_IMA_ADPCM 0x0011
#define WAVE_FORMAT_MPEG 0x0050
#define WAVE_FORMAT_MPEGLAYER3 0x0055
#define WAVE_FORMAT_OGGVORBIS 0x6750
#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_XBOX_ADPCM 0x0069

namespace FMOD {

class SyncPoint;

#pragma pack(push, 1)

struct WAVE_CHUNK {
        int8_t id[4];
        uint32_t size;
};

struct WAVE_FORMATEX {
        uint16_t wFormatTag;
        uint16_t nChannels;
        uint32_t nSamplesPerSec;
        uint32_t nAvgBytesPerSec;
        uint16_t nBlockAlign;
        uint16_t wBitsPerSample;
        uint16_t cbSize;
};

struct WAVE_FORMATEXTENSIBLE {
        WAVE_FORMATEX Format;
        union {
                uint16_t wValidBitsPerSample;
                uint16_t wSamplesPerBlock;
                uint16_t wReserved;
        } Samples;
        uint32_t dwChannelMask;
        FMOD_GUID SubFormat;
};

struct WAVE_SMPLHEADER {
        uint32_t Manufacturer;
        uint32_t Product;
        uint32_t SamplePeriod;
        uint32_t Note;
        uint32_t FineTune;
        uint32_t SMPTEFormat;
        uint32_t SMPTEOffset;
        uint32_t Loops;
        uint32_t SamplerData;
        struct {
                uint32_t Identifier;
                uint32_t Type;
                uint32_t Start;
                uint32_t End;
                uint32_t Fraction;
                uint32_t Count;
        } Loop;
};

struct WAVE_CUEPOINT {
        int32_t dwIdentifier;
        int32_t dwPosition;
        char fccChunk[4];
        int32_t dwChunkStart;
        int32_t dwBlockStart;
        int32_t dwSampleOffset;
};

struct WAVE_FORMAT_IMAADPCM {
        WAVE_FORMATEX wfx;
        uint16_t wSamplesPerBlock;
};

#pragma pack(pop)

// confirm that our attempts to pack the WAVE structures were successful:
static_assert(sizeof(WAVE_CHUNK) == 8, "WAVE_CHUNK must be 8 bytes long");
static_assert(sizeof(WAVE_FORMATEX) == 18, "WAVE_FORMATEX must be 18 bytes long");
static_assert(sizeof(WAVE_FORMAT_IMAADPCM) == 20, "WAVE_FORMAT_IMAADPCM must be 20 bytes long");
static_assert(sizeof(WAVE_FORMATEXTENSIBLE) == 40, "WAVE_FORMATEXTENSIBLE must be 40 bytes long");
static_assert(sizeof(WAVE_SMPLHEADER) == 60, "WAVE_SMPLHEADER must be 60 bytes long");
static_assert(sizeof(WAVE_CUEPOINT) == 24, "WAVE_CUEPOINT must be 24 bytes long");

class CodecWav : public Codec {
        friend class CodecMPEG;
        friend class CodecRaw;
        friend class CodecOggVorbis;
        friend class ChannelSoftware;
        friend class DSPCodec;
        friend class DSPCodecPool;
        friend class SystemI;

    private:
        // HACMSTREAM mACMCodec;
        WAVE_FORMATEXTENSIBLE mDestFormat;
        uint32_t mNumSyncPoints;
        SyncPoint* mSyncPoint;
        int32_t mSamplesPerADPCMBlock;
        WAVE_FORMATEXTENSIBLE mSrcFormatMemory;
        WAVE_FORMATEXTENSIBLE* mSrcFormat;

    public:
        FMOD_RESULT openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        FMOD_RESULT closeInternal();
        FMOD_RESULT readInternal(void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        FMOD_RESULT soundCreateInternal(int32_t subsound, FMOD_SOUND* sound);
        FMOD_RESULT canPointInternal();
        FMOD_RESULT setPositionInternal(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);
        FMOD_RESULT parseChunk(uint32_t chunksize);

        static FMOD_RESULT F_CALLBACK openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK soundCreateCallback(FMOD_CODEC_STATE* codec, int32_t subsound, FMOD_SOUND* sound);
        static FMOD_RESULT F_CALLBACK canPointCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        static FMOD_RESULT F_CALLBACK setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);
        static FMOD_CODEC_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
