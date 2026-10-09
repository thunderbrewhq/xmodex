#include "xmodex/sound/Stream.hpp"
#include "xmodex/channel/ChannelStream.hpp"
#include "xmodex/codec/Codec.hpp"
#include "xmodex/file/File.hpp"
#include "xmodex/sound/Sample.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

Stream::Stream() {
    this->mLastPos = 0;
    this->mSubSound = nullptr;
    this->mLoopCountCurrent = -1;
    this->mBlockSize = 1;
    this->mChannel = nullptr;
    this->mInitialPosition = 0;
}

FMOD_RESULT Stream::fill(uint32_t offset, uint32_t length) {
    FMOD_RESULT result = FMOD_OK;
    uint32_t read = 0;

    if (!(this->mFlags & FMOD_SOUND_FLAG_FINISHED)) {
        uint32_t size;
        uint32_t len = length;
        uint32_t r;

        do {
            Stream* stream;
            int32_t streamlength;
            uint32_t endpoint;

            if (this->mSubSound) {
                stream = static_cast<Stream*>(this->mSubSound[this->mSubSoundList ? this->mSubSoundList[this->mSubSoundListCurrent] : this->mSubSoundListCurrent]);

                if (!stream) {
                    break;
                }

                if (this->mSubSoundShared) {
                    FMOD_CODEC_WAVEFORMAT waveformat;
                    if (this->mSubSoundList) {
                        this->mCodec->mDescription.getwaveformat(
                            this->mCodec,
                            this->mSubSoundList[this->mSubSoundListCurrent],
                            &waveformat);
                        streamlength = waveformat.lengthpcm;
                    } else {
                        this->mCodec->mDescription.getwaveformat(
                            this->mCodec,
                            this->mSubSoundIndex,
                            &waveformat);
                        streamlength = waveformat.lengthpcm;
                    }
                } else {
                    streamlength = stream->mLength;
                }
            } else {
                streamlength = this->mLength;
                stream = this;
            }

            if (this->mMode & FMOD_LOOP_NORMAL && this->mLoopCountCurrent) {
                endpoint = stream->mLoopStart + stream->mLoopLength - 1;
            } else {
                if (this->mSample->mCodec->mFlags & FMOD_CODEC_ACCURATELENGTH) {
                    endpoint = streamlength - 1;
                } else {
                    endpoint = static_cast<uint32_t>(-1);
                }
            }

            size = len;

            if (offset + size > this->mSample->mLength) {
                size = this->mSample->mLength - offset;
            }

            if (stream->mPosition > endpoint) {
                size = 0;
            } else if (stream->mPosition + size > endpoint) {
                size = (endpoint - stream->mPosition) + 1;
            }

            result = this->mSample->read(offset, size, &r);
            if (result == FMOD_OK) {
                if (r == 0) {
                    break;
                }
            } else if (result != FMOD_ERR_FILE_EOF) {
                return result;
            }

            read += r;
            len -= r;
            offset += r;
            if (this->mSample->mLength <= offset) {
                offset = 0;
            }

            stream->mLastPos = stream->mPosition;
            stream->mPosition += r;

            if (endpoint < stream->mPosition || result == FMOD_ERR_FILE_EOF) {
                if (this->mSubSoundList) {
                    bool finished = false;
                    bool advance = false;
                    do {
                        this->mSubSoundListCurrent++;

                        if (this->mSubSoundListNum <= this->mSubSoundListCurrent) {
                            if (!(this->mMode & FMOD_LOOP_NORMAL & this->mLoopCountCurrent)) {
                                finished = true;
                            }

                            this->mSubSoundListCurrent = 0;
                        }

                        this->mSubSoundIndex = this->mSubSoundList[this->mSubSoundListCurrent];

                        stream = static_cast<Stream*>(this->mSubSound[this->mSubSoundList[this->mSubSoundListCurrent]]);

                        if (stream) {
                            this->mSample->mCodec = stream->mCodec;

                            if (!finished) {
                                int32_t subsound;

                                if (this->mCodec == this->mSample->mCodec) {
                                    subsound = this->mSubSoundIndex;
                                } else {
                                    subsound = -1;
                                }

                                result = this->mSample->seek(subsound, 0);
                            }

                            stream->mPosition = 0;

                            if (!finished) {
                                advance = true;
                            }

                            break;
                        }
                    } while (!finished);

                    if (advance) {
                        continue;
                    }
                }

                if (!(this->mMode & FMOD_LOOP_NORMAL && this->mLoopCountCurrent)) {
                    if (stream != this) {
                        this->mFlags |= FMOD_SOUND_FLAG_FINISHED;
                        this->mPosition = this->mLength;
                    }
                    stream->mFlags |= FMOD_SOUND_FLAG_FINISHED;
                    stream->mPosition = streamlength;
                    break;
                }

                size = this->mLoopStart;
                stream->mPosition = size;

                if (this->mLength != static_cast<uint32_t>(-1)) {
                    result = this->mSample->seek(this->mSubSoundIndex, size);
                    if (result != FMOD_OK) {
                        return result;
                    }
                    stream->mPosition = this->mSample->mPosition;
                    result = FMOD_OK;
                }

                if (this->mLoopCountCurrent > 0) {
                    this->mLoopCountCurrent--;
                }
            } else if (!r) {
                break;
            }
        } while (len);
    }

    if (read < length) {
        uint32_t size;
        uint32_t len;

        len = length - read;
        do {
            size = len;
            if (offset + size > this->mSample->mLength) {
                size = this->mSample->mLength - offset;
            }

            this->mSample->clear(offset, size);

            len -= size;
            offset += size;
            if (offset >= this->mSample->mLength) {
                offset = 0;
            }
        } while (len);
    }

    return result;
}

FMOD_RESULT Stream::flush() {
    FMOD_RESULT result;

    result = this->fill(0, this->mSample->mLength);

    if (result == FMOD_OK || result == FMOD_ERR_FILE_EOF) {
        this->mFlags &= ~FMOD_SOUND_FLAG_WANTSTOFLUSH;
        result = FMOD_OK;
    }

    return result;
}

FMOD_RESULT Stream::getPosition(uint32_t* position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;

    if (this->mOpenState != FMOD_OPENSTATE_READY) {
        return FMOD_ERR_NOTREADY;
    }

    if (!position) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (postype == static_cast<FMOD_TIMEUNIT>(FMOD_TIMEUNIT_SENTENCE_SUBSOUND | FMOD_TIMEUNIT_BUFFERED)) {
        *position = this->mSubSoundListCurrent;
    } else if (postype == FMOD_TIMEUNIT_PCM) {
        *position = this->mLastPos;
    } else if (postype == FMOD_TIMEUNIT_PCMBYTES) {
        // XXX:
    } else if (postype == FMOD_TIMEUNIT_MS) {
        *position = static_cast<uint32_t>(static_cast<float>(this->mLastPos) / 1000.0f * this->mDefaultFrequency);
    } else {
        result = this->mCodec->getPosition(position, postype);
        if (result != FMOD_OK) {
            return result;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT Stream::setPosition(uint32_t position, FMOD_TIMEUNIT postype) {
    FMOD_RESULT result;
    bool seekable = true;
    uint32_t endpoint;

    if (postype == FMOD_TIMEUNIT_PCM) {
        if (this->mMode & FMOD_LOOP_OFF) {
            endpoint = this->mLength - 1;
        } else {
            endpoint = (this->mLoopLength - 1) + this->mLoopStart;
        }
        if (endpoint < position) {
            return FMOD_ERR_INVALID_PARAM;
        }
    }

    if (this->mCodec->mFile) {
        seekable = this->mCodec->mFile->mSeekable;
    }

    this->mFlags &= ~FMOD_SOUND_FLAG_FINISHED;

    if (seekable) {
        if (this->mSubSound) {
            if (this->mSubSoundList && postype == FMOD_TIMEUNIT_PCM) {
                uint32_t offset = 0;
                for (int32_t i = 0; i < this->mSubSoundListNum; i++) {
                    auto subsound = this->mSubSound[this->mSubSoundList[i]];
                    if (subsound) {
                        FMOD_CODEC_WAVEFORMAT waveformat;

                        uint32_t lengthpcm;

                        if (!this->mSubSoundList) {
                            return FMOD_ERR_INVALID_PARAM;
                        }

                        if (this->mSubSoundShared) {
                            result = this->mCodec->mDescription.getwaveformat(
                                this->mCodec,
                                this->mSubSoundList[i],
                                &waveformat);
                            lengthpcm = waveformat.lengthpcm;
                            if (result != FMOD_OK) {
                                return result;
                            }
                        } else {
                            lengthpcm = subsound->mLength;
                        }

                        if (offset <= position && position < (offset + lengthpcm)) {
                            auto substream = static_cast<Stream*>(subsound);

                            this->mSubSoundListCurrent = i;
                            this->mSubSoundIndex = this->mSubSoundList[i];
                            substream->mSubSoundIndex = this->mSubSoundList[i];
                            substream->updateSubSound(this->mSubSoundIndex, true);
                            substream->setPosition(position - offset, FMOD_TIMEUNIT_PCM);
                            break;
                        }

                        offset += lengthpcm;
                    }
                }
            } else if (this->mSubSound && postype == FMOD_TIMEUNIT_PCM) {
                auto subsound = this->mSubSound[this->mSubSoundIndex];

                if (subsound) {
                    auto substream = static_cast<Stream*>(subsound);

                    result = substream->setPosition(position, postype);
                }
            } else {
                this->mCodec->reset();

                result = this->mCodec->setPosition(this->mSubSoundIndex, position, postype);
                if (result != FMOD_OK) {
                    return result;
                }

                if (this->mSubSoundParent) {
                    this->mSubSoundParent->mSubSoundIndex = this->mSubSoundIndex;
                }
            }

            if (this->mSample->mPostSetPositionCallback) {
                this->mSample->mPostSetPositionCallback(
                    reinterpret_cast<FMOD_SOUND*>(this),
                    this->mSubSoundIndex,
                    position,
                    postype);
            }

            if (postype != FMOD_TIMEUNIT_MS && postype != FMOD_TIMEUNIT_PCM && postype != FMOD_TIMEUNIT_PCMBYTES) {
                position = 0;
            }

            this->mPosition = position;
            this->mLastPos = position;
        }
    } else {
        if (this->mLastPos && position) {
            return FMOD_ERR_FILE_COULDNOTSEEK;
        }
    }

    return FMOD_OK;
}

bool Stream::isStream() {
    return true;
}

FMOD_RESULT Stream::setLoopCount(int32_t loopcount) {
    if (this->mOpenState != FMOD_OPENSTATE_READY) {
        return FMOD_ERR_NOTREADY;
    }

    this->mLoopCountCurrent = loopcount;
    this->mLoopCount = loopcount;

    return FMOD_OK;
}

} // namespace FMOD
