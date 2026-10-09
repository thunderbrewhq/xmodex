#include "xmodex/codec/CodecOggVorbis.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/file/File.hpp"
#include <vorbis/vorbisfile.h>
#include <vorbis/window.h>

namespace FMOD {

bool CodecOggVorbis::gInitialized = false;

FMOD_CODEC_DESCRIPTION_EX oggvorbiscodec;

size_t FMOD_OggVorbis_ReadCallback(void* ptr, size_t size, size_t nmemb, void* datasource) {
    FMOD_RESULT result;
    uint32_t rd;

    auto fp = static_cast<File*>(datasource);

    result = fp->read(ptr, static_cast<uint32_t>(size), static_cast<uint32_t>(nmemb), &rd);
    if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
        return static_cast<size_t>(-1);
    }

    return rd;
}

int FMOD_OggVorbis_SeekCallback(void* datasource, ogg_int64_t offset, int whence) {
    auto fp = static_cast<File*>(datasource);

    if (fp->mSeekable) {
        return fp->seek(static_cast<int32_t>(offset), whence);
    } else {
        return -1;
    }
}

long FMOD_OggVorbis_TellCallback(void* datasource) {
    auto fp = static_cast<File*>(datasource);
    uint32_t pos;

    fp->tell(&pos);

    return static_cast<long>(pos);
}

FMOD_RESULT CodecOggVorbis::closeInternal() {
    this->mVf.datasource = nullptr;

    ov_clear(&this->mVf);

    return FMOD_OK;
}

FMOD_RESULT CodecOggVorbis::openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    FMOD_RESULT result;
    uint32_t lengthbytes = 0;
    char str[4];
    bool manualsizecalc = false;
    int32_t oggresult;
    ov_callbacks callbacks = {
        FMOD_OggVorbis_ReadCallback,
        FMOD_OggVorbis_SeekCallback,
        nullptr,
        FMOD_OggVorbis_TellCallback,
    };

    this->init(FMOD_SOUND_TYPE_OGGVORBIS);

    result = this->mFile->seek(0, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    this->mSrcDataOffset = 0;

    {
        CodecWav tempwav;
        FMOD_CODEC_WAVEFORMAT tempwaveformat;
        WAVE_CHUNK chunk;
        FMOD_memset(&tempwav, 0, sizeof(CodecWav));
        FMOD_memset(&tempwaveformat, 0, sizeof(FMOD_CODEC_WAVEFORMAT));

        tempwav.mFile = this->mFile;
        tempwav.waveformat = &tempwaveformat;
        tempwav.mSrcDataOffset = static_cast<uint32_t>(-1);

        result = tempwav.mFile->read(&chunk, 1, sizeof(WAVE_CHUNK), nullptr);
        if (!FMOD_strncmp((const char*)chunk.id, "RIFF", 4)) {
            char wave[4];

            result = this->mFile->read(wave, 1, 4, 0);
            if (result != FMOD_OK) {
                return result;
            }

            if (!FMOD_strncmp(wave, "WAVE", 4)) {
#if defined(XMODEX_CPU_BIGENDIAN)
                chunk.size = FMOD_SWAPENDIAN_DWORD(chunk.size);
#endif

                result = tempwav.parseChunk(chunk.size);
                if (result == FMOD_OK && tempwav.mSrcFormat && tempwav.mSrcDataOffset == static_cast<uint32_t>(-1)) {
                    int32_t format = tempwav.mSrcFormat->Format.wFormatTag;

                    if (format == WAVE_FORMAT_OGGVORBIS) {
                        this->mSrcDataOffset = tempwav.mSrcDataOffset;
                        lengthbytes = tempwav.waveformat[0].lengthbytes;
                        this->mLoopPoints[0] = tempwav.mLoopPoints[0];
                        this->mLoopPoints[1] = tempwav.mLoopPoints[1];
                        this->mSyncPoint = tempwav.mSyncPoint;
                        this->mNumSyncPoints = tempwav.mNumSyncPoints;
                    } else {
                        result = FMOD_ERR_FORMAT;
                    }

                    if (result != FMOD_OK) {
                        if (tempwav.mSrcFormat) {
                            FMOD_Memory_Free(tempwav.mSrcFormat);
                            tempwav.mSrcFormat = nullptr;
                        }
                        if (tempwav.mSyncPoint && mSyncPoint != tempwav.mSyncPoint) {
                            FMOD_Memory_Free(tempwav.mSyncPoint);
                            tempwav.mSyncPoint = nullptr;
                        }
                        return result;
                    }
                }

                if (tempwav.mSrcFormat) {
                    FMOD_Memory_Free(tempwav.mSrcFormat);
                    tempwav.mSrcFormat = nullptr;
                }

                if (tempwav.mSyncPoint && mSyncPoint != tempwav.mSyncPoint) {
                    FMOD_Memory_Free(tempwav.mSyncPoint);
                    tempwav.mSyncPoint = nullptr;
                }
            }
        }
    }

    this->mFile->seek(this->mSrcDataOffset, SEEK_SET);

    result = this->mFile->read(str, 1, 4, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    if (FMOD_strncmp(str, "OggS", 4)) {
        return FMOD_ERR_FORMAT;
    }

    if (!gInitialized) {
        _vorbis_window_init();
        gInitialized = true;
    }

    if (!lengthbytes) {
        result = this->mFile->getSize(&lengthbytes);
        if (result != FMOD_OK) {
            return result;
        }

        manualsizecalc = true;
    }

    result = this->mFile->seek(this->mSrcDataOffset, SEEK_SET);
    if (result != FMOD_OK) {
        return result;
    }

    oggresult = ov_open_callbacks(this->mFile, &this->mVf, nullptr, 0, callbacks);
    if (oggresult < 0) {
        if (oggresult == OV_EVERSION) {
            return FMOD_ERR_FORMAT;
        } else if (oggresult == OV_EMEMORY) {
            return FMOD_ERR_MEMORY;
        }
        return FMOD_ERR_FORMAT;
    }

    auto vi = ov_info(&this->mVf, -1);

    result = this->readVorbisComments();
    if (result != FMOD_OK) {
        return result;
    }

    this->waveformat = &this->mWaveFormat;
    this->waveformat[0].lengthbytes = lengthbytes;
    this->waveformat[0].format = FMOD_SOUND_FORMAT_PCM16;
    this->waveformat[0].channels = vi->channels;
    this->waveformat[0].frequency = vi->rate;
    this->waveformat[0].blockalign = this->waveformat[0].channels * 2;

    if (manualsizecalc && this->waveformat[0].lengthbytes != static_cast<uint32_t>(-1)) {
        this->waveformat[0].lengthbytes -= this->mSrcDataOffset;
    }

    if (this->mFile->mSeekable) {
        auto streams = ov_streams(&this->mVf);

        this->waveformat[0].lengthpcm = 0;

        for (int32_t i = 0; i < streams; i++) {
            this->waveformat[0].lengthpcm += static_cast<int32_t>(ov_pcm_total(&this->mVf, i));
        }

        if (this->waveformat[0].lengthpcm <= 0) {
            this->waveformat[0].lengthpcm = 0;

            return FMOD_ERR_FORMAT;
        }
    } else {
        this->waveformat[0].lengthpcm = 0x7fffffff;
    }

    if (!this->mSrcDataOffset) {
        this->mSrcDataOffset = static_cast<uint32_t>(ov_raw_tell(&this->mVf));
    }

    this->numsubsounds = 0;

    return result;
}

FMOD_RESULT CodecOggVorbis::readInternal(void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    int32_t bigendian;
    vorbis_comment* vc;
    char* name;
    char* value;

#if defined(XMODEX_CPU_BIGENDIAN)
    bigendian = 1;
#else
    bigendian = 0;
#endif

    *bytesread = ov_read(&this->mVf, reinterpret_cast<char*>(buffer), sizebytes, bigendian, 2, 1, nullptr);
    if (!*bytesread) {
        return FMOD_ERR_FILE_EOF;
    }

    vc = ov_comment(&this->mVf, -1);

    if (vc && vc->comments) {
        for (int32_t i = 0; i < vc->comments; i++) {
            name = vc->user_comments[i];
            value = name;
            for (; *value && (*value != '='); value++)
                ;

            if (*value == '=') {
                *value++ = 0;
            } else {
                value = name;
                name = const_cast<char*>("NONAME");
            }

            this->metadata(
                static_cast<FMOD_CODEC_STATE*>(this),
                FMOD_TAGTYPE_VORBISCOMMENT,
                name,
                value,
                FMOD_strlen(value) + 1,
                FMOD_TAGDATATYPE_STRING,
                true);
        }

        vorbis_comment_clear(vc);
    }

    return FMOD_OK;
}

FMOD_RESULT CodecOggVorbis::readVorbisComments() {
    FMOD_RESULT result;
    char* p;

    auto vc = ov_comment(&this->mVf, -1);
    if (!vc) {
        return FMOD_OK;
    }

    for (int32_t i = 0; i < vc->comments; i++) {
        if (vc->comment_lengths[i]) {
            p = vc->user_comments[i];
            for (int32_t j = 0; *p && (*p != '='); j++, p++)
                ;
            if (*p == '=') {
                *p++ = 0;
                result = this->metaData(
                    FMOD_TAGTYPE_VORBISCOMMENT,
                    vc->user_comments[i],
                    p,
                    FMOD_strlen(p) + 1,
                    FMOD_TAGDATATYPE_STRING,
                    false);
                if (result != FMOD_OK) {
                    return result;
                }
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT CodecOggVorbis::setPositionInternal(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    int32_t ret = ov_pcm_seek(&this->mVf, static_cast<ogg_int64_t>(position));
    if (ret < 0) {
        return FMOD_ERR_MEMORY;
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK CodecOggVorbis::closeCallback(FMOD_CODEC_STATE* codec) {
    auto ogg = static_cast<CodecOggVorbis*>(codec);
    return ogg->closeInternal();
}

FMOD_RESULT F_CALLBACK CodecOggVorbis::openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo) {
    auto ogg = static_cast<CodecOggVorbis*>(codec);
    return ogg->openInternal(usermode, userexinfo);
}

FMOD_RESULT F_CALLBACK CodecOggVorbis::readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    auto ogg = static_cast<CodecOggVorbis*>(codec);
    return ogg->readInternal(buffer, sizebytes, bytesread);
}

FMOD_RESULT F_CALLBACK CodecOggVorbis::setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    auto ogg = static_cast<CodecOggVorbis*>(codec);
    return ogg->setPositionInternal(subsound, position, postype);
}

FMOD_CODEC_DESCRIPTION_EX* CodecOggVorbis::getDescriptionEx() {
    FMOD_memset(&oggvorbiscodec, 0, sizeof(FMOD_CODEC_DESCRIPTION_EX));

    oggvorbiscodec.name = "FMOD Ogg Vorbis Codec";
    oggvorbiscodec.version = 0x10100;
    oggvorbiscodec.timeunits = FMOD_TIMEUNIT_PCM;

    oggvorbiscodec.open = CodecOggVorbis::openCallback;
    oggvorbiscodec.close = CodecOggVorbis::closeCallback;
    oggvorbiscodec.read = CodecOggVorbis::readCallback;
    oggvorbiscodec.setposition = CodecOggVorbis::setPositionCallback;

    oggvorbiscodec.mType = FMOD_SOUND_TYPE_OGGVORBIS;
    oggvorbiscodec.mSize = sizeof(CodecOggVorbis);

    return &oggvorbiscodec;
}

} // namespace FMOD
