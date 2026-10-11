#include "xmodex/codec/Codec.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/file/File.hpp"

namespace FMOD {

Codec::Codec() {
    this->mType = FMOD_SOUND_TYPE_UNKNOWN;
    this->mMetadata = nullptr;
    this->fileread = Codec::defaultFileRead;
    this->fileseek = Codec::defaultFileSeek;
    this->metadata = Codec::defaultMetaData;
}

FMOD_RESULT Codec::init(FMOD_SOUND_TYPE type) {
    Plugin::init();

    this->mType = type;
    this->mMetadata = nullptr;
    this->numsubsounds = 0;
    this->waveformat = nullptr;

    return FMOD_OK;
}

bool Codec::readyForRelease() {
    if (this->mFile) {
        if (!this->mFile->readyForClose()) {
            return false;
        }
    }

    return true;
}

FMOD_RESULT Codec::release() {
    if (this->mDescription.close) {
        this->mDescription.close(this);
    }

    if (this->mFile) {
        this->mFile->close();
        FMOD_Memory_Free(this->mFile);
        this->mFile = nullptr;
    }

    if (this->mWaveFormatMemory && this->mType == FMOD_SOUND_TYPE_FSB) {
        FMOD_Memory_Free(this->mWaveFormatMemory);
        this->mWaveFormatMemory = nullptr;
    }

    if (this->mMetadata) {
        this->mMetadata->release();
        this->mMetadata = nullptr;
    }

    return Plugin::release();
}

FMOD_RESULT Codec::canPointTo() {
    if (this->mDescription.canpoint) {
        return this->mDescription.canpoint(this);
    }

    return FMOD_ERR_MEMORY_CANTPOINT;
}

FMOD_RESULT Codec::getLength(uint32_t* length, FMOD_TIMEUNIT lengthtype) {
    FMOD_RESULT result;
    FMOD_CODEC_WAVEFORMAT waveformat;

    if (lengthtype == FMOD_TIMEUNIT_RAWBYTES) {
        result = this->mDescription.getwaveformat(
            this,
            this->mSubSoundIndex,
            &waveformat);
        if (result != FMOD_OK) {
            return result;
        }

        *length = waveformat.lengthbytes;
    } else {
        if (!this->mDescription.getlength) {
            *length = 0;
            return FMOD_ERR_UNSUPPORTED;
        }

        result = this->mDescription.getlength(
            this,
            length,
            lengthtype);
    }

    return result;
}

FMOD_RESULT Codec::getMetadataFromFile() {
    FMOD_RESULT result = FMOD_OK;
    Metadata* filemetadata;

    if (this->mFile) {
        result = this->mFile->getMetadata(&filemetadata);
        if (result == FMOD_OK) {
            if (!this->mMetadata) {
                this->mMetadata = FMOD_Object_Alloc(Metadata);
                if (!this->mMetadata) {
                    return FMOD_ERR_MEMORY;
                }
            }

            result = this->mMetadata->add(filemetadata);
        }
    }

    return result;
}

FMOD_RESULT Codec::getPosition(uint32_t* position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;

    if (postype == FMOD_TIMEUNIT_RAWBYTES) {
        if (!this->mFile) {
            *position = 0;
        }

        result = this->mFile->tell(position);
        if (result != FMOD_OK) {
            *position = 0;
            return result;
        }

        *position -= this->mSrcDataOffset;
    }

    if (!this->mDescription.getposition) {
        return FMOD_ERR_UNSUPPORTED;
    }

    if (!(postype & this->mDescription.timeunits)) {
        return FMOD_ERR_FORMAT;
    }

    result = this->mDescription.getposition(this, position, postype);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT Codec::metaData(FMOD_TAGTYPE type, const char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype, bool unique) {
    if (!this->mMetadata) {
        this->mMetadata = FMOD_Object_Alloc(Metadata);
        if (!this->mMetadata) {
            return FMOD_ERR_MEMORY;
        }
    }

    return this->mMetadata->addTag(type, name, data, datalen, datatype, unique);
}

FMOD_RESULT Codec::read(void* buffer, uint32_t sizebytes, uint32_t* bytesread) {
    FMOD_RESULT result = FMOD_OK;
    bool checkmetadata = false;
    uint32_t read = 0;

    if (this->mPCMBuffer && this->mPCMBufferLengthBytes) {
        while (sizebytes) {
            uint32_t lenbytes = sizebytes;
            uint32_t bytesdecoded = 0;

            if (!this->mPCMBufferOffsetBytes) {
                result = this->mDescription.read(this, this->mPCMBuffer, this->mPCMBufferLengthBytes, &bytesdecoded);
                if (result != FMOD_OK) {
                    break;
                }

                this->mPCMBufferFilledBytes = bytesdecoded;

                lenbytes = bytesdecoded;
                if (lenbytes > sizebytes) {
                    lenbytes = sizebytes;
                }

                checkmetadata = true;
            }

            if (this->mPCMBufferOffsetBytes + lenbytes > this->mPCMBufferFilledBytes) {
                lenbytes = this->mPCMBufferFilledBytes - this->mPCMBufferOffsetBytes;
            }

            FMOD_memcpy(reinterpret_cast<char*>(buffer) + read, this->mPCMBuffer + this->mPCMBufferOffsetBytes, lenbytes);

            this->mPCMBufferOffsetBytes += lenbytes;
            if (this->mPCMBufferOffsetBytes >= this->mPCMBufferFilledBytes) {
                this->mPCMBufferOffsetBytes = 0;
            }

            if (!lenbytes) {
                lenbytes = sizebytes;
                break;
            }

            sizebytes -= lenbytes;
            read += lenbytes;
        }
    } else {
        result = this->mDescription.read(this, buffer, sizebytes, &read);
        if (result == FMOD_OK) {
            checkmetadata = true;
        }
    }

    if (checkmetadata) {
        this->getMetadataFromFile();
    }

    if (bytesread) {
        *bytesread = read;
    }

    return result;
}

FMOD_RESULT Codec::reset() {
    this->mPCMBufferOffsetBytes = 0;

    if (this->mPCMBuffer) {
        FMOD_memset(this->mPCMBuffer, 0, this->mPCMBufferLengthBytes);
    }

    if (this->mDescription.reset) {
        return this->mDescription.reset(this);
    }

    return FMOD_OK;
}

FMOD_RESULT Codec::setPosition(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;
    FMOD_CODEC_WAVEFORMAT waveformat;

    if (numsubsounds && subsound >= numsubsounds) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mDescription.setposition) {
        return FMOD_ERR_UNSUPPORTED;
    }

    if (subsound < 0) {
        subsound = this->mSubSoundIndex;
    }

    if (!numsubsounds) {
        subsound = 0;
    }

    result = this->mDescription.getwaveformat(this, subsound, &waveformat);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDescription.timeunits & FMOD_TIMEUNIT_PCM) {
        if (postype & FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getSamplesFromBytes(position, &position, waveformat.channels, waveformat.format);
            postype = FMOD_TIMEUNIT_PCM;
        } else if (postype & FMOD_TIMEUNIT_MS) {
            position = static_cast<uint32_t>(static_cast<float>(position) / 1000.0f * waveformat.frequency);
            postype = FMOD_TIMEUNIT_PCM;
        }
    } else if (this->mDescription.timeunits & FMOD_TIMEUNIT_PCMBYTES) {
        if (postype & FMOD_TIMEUNIT_PCM) {
            SoundI::getBytesFromSamples(position, &position, waveformat.channels, waveformat.format);
            postype = FMOD_TIMEUNIT_PCMBYTES;
        } else if (postype & FMOD_TIMEUNIT_MS) {
            position = static_cast<uint32_t>(static_cast<float>(position) / 1000.0f * waveformat.frequency);
            SoundI::getBytesFromSamples(position, &position, waveformat.channels, waveformat.format);
            postype = FMOD_TIMEUNIT_PCMBYTES;
        }
    } else if (this->mDescription.timeunits & FMOD_TIMEUNIT_MS) {
        if (postype & FMOD_TIMEUNIT_PCM) {
            position = static_cast<uint32_t>(static_cast<float>(position) / waveformat.frequency * 1000.0f);
            postype = FMOD_TIMEUNIT_MS;
        } else if (postype & FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getSamplesFromBytes(position, &position, waveformat.channels, waveformat.format);
            position = static_cast<uint32_t>(static_cast<float>(position) / waveformat.frequency * 1000.0f);
            postype = FMOD_TIMEUNIT_MS;
        }
    }

    if (!(postype & this->mDescription.timeunits)) {
        return FMOD_ERR_FORMAT;
    }

    this->mPCMBufferOffsetBytes = 0;

    result = this->mDescription.setposition(this, subsound, position, postype);
    if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
        return result;
    }

    this->mSubSoundIndex = subsound;

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK Codec::defaultFileRead(void* handle, void* buffer, uint32_t size, uint32_t* read, void* userdata) {
    return static_cast<File*>(handle)->read(buffer, 1, size, read);
}

FMOD_RESULT F_CALLBACK Codec::defaultFileSeek(void* handle, uint32_t pos, void* userdata) {
    return static_cast<File*>(handle)->seek(pos, 0);
}

FMOD_RESULT F_CALLBACK Codec::defaultGetWaveFormat(FMOD_CODEC_STATE* codec, int32_t index, FMOD_CODEC_WAVEFORMAT* waveformat_out) {
    if (!codec->waveformat) {
        return FMOD_ERR_INTERNAL;
    }

    if (index < 0 || (codec->numsubsounds == 0 && index > 0) || (codec->numsubsounds > 0 && index >= codec->numsubsounds)) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_memcpy(
        waveformat_out,
        &codec->waveformat[index],
        sizeof(FMOD_CODEC_WAVEFORMAT));

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK Codec::defaultMetaData(FMOD_CODEC_STATE* codec, FMOD_TAGTYPE type, char* name, void* data, uint32_t datalen, FMOD_TAGDATATYPE datatype, int32_t unique) {
    return static_cast<Codec*>(codec)->metaData(type, name, data, datalen, datatype, unique == 1 ? true : false);
}

} // namespace FMOD
