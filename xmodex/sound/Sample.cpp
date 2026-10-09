#include "xmodex/sound/Sample.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

Sample::Sample() {
    this->mNumSubSamples = 0;
    this->mLockCanRead = true;
}

FMOD_RESULT Sample::release(bool freethis) {
    if (this->mSystem) {
        this->mSystem->stopSound(this);

        if (this->mLockBuffer) {
            this->mSystem->mMultiSubSampleLockBuffer.free("", 0);
        }
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        if (this->mSubSample[i]) {
            this->mSubSample[i]->mCodec = nullptr;
            this->mSubSample[i]->release(true);
            this->mSubSample[i] = nullptr;
        }
    }

    return SoundI::release(freethis);
}

FMOD_RESULT Sample::lock(
    uint32_t offset,
    uint32_t length,
    void** ptr1,
    void** ptr2,
    uint32_t* len1,
    uint32_t* len2) {
    uint32_t samples = 0;
    uint32_t lenbytes;

    if (!ptr1 || !len1) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mNumSubSamples < 1) {
        return this->lockInternal(offset, length, ptr1, ptr2, len1, len2);
    }

    if (this->mMode & FMOD_CREATECOMPRESSEDSAMPLE) {
        return FMOD_ERR_FORMAT;
    }

    this->getBytesFromSamples(this->mLength, &lenbytes);
    if (length + offset > lenbytes) {
        length = lenbytes - offset;
    }

    length = length > SOUND_READCHUNKSIZE ? SOUND_READCHUNKSIZE : length;

    FMOD_OS_CriticalSection_Enter(this->mSystem->mMultiSubSampleLockBufferCrit);

    *ptr1 = this->mLockBuffer;
    if (ptr2) {
        *ptr2 = nullptr;
    }
    *len1 = length;
    if (len2) {
        *len2 = 0;
    }

    this->mLockLength = length;
    this->mLockOffset = offset;

    SoundI::getSamplesFromBytes(length, &samples);

    length /= this->mNumSubSamples;
    offset /= this->mNumSubSamples;

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        if (this->mSubSample[i]->mLockCanRead) {
            uint32_t in_len1;
            uint32_t in_len2;
            void* in_ptr1;
            void* in_ptr2;

            this->mSubSample[i]->lock(offset, length, &in_ptr1, &in_ptr2, &in_len1, &in_len2);

            switch (this->mFormat) {
            case FMOD_SOUND_FORMAT_PCM8: {
                auto dest = reinterpret_cast<int8_t*>(*ptr1) + i;
                auto src = reinterpret_cast<int8_t*>(in_ptr1);

                uint32_t count2 = samples >> 3;
                while (count2) {
                    dest[0] = src[0];
                    dest[this->mNumSubSamples * 1] = src[1];
                    dest[this->mNumSubSamples * 2] = src[2];
                    dest[this->mNumSubSamples * 3] = src[3];
                    dest[this->mNumSubSamples * 4] = src[4];
                    dest[this->mNumSubSamples * 5] = src[5];
                    dest[this->mNumSubSamples * 6] = src[6];
                    dest[this->mNumSubSamples * 7] = src[7];

                    dest += this->mNumSubSamples * 8;
                    src += 8;
                    count2--;
                }

                count2 = samples & 7;
                while (count2) {
                    dest[0] = src[0];

                    dest += this->mNumSubSamples;
                    src++;
                    count2--;
                }
                break;
            }
            case FMOD_SOUND_FORMAT_GCADPCM: {
                samples = length / 2;
            }
            case FMOD_SOUND_FORMAT_PCM16: {
                auto dest = reinterpret_cast<int16_t*>(*ptr1) + i;
                auto src = reinterpret_cast<int16_t*>(in_ptr1);

                uint32_t count2 = samples >> 3;
                while (count2) {
                    dest[0] = src[0];
                    dest[this->mNumSubSamples * 1] = src[1];
                    dest[this->mNumSubSamples * 2] = src[2];
                    dest[this->mNumSubSamples * 3] = src[3];
                    dest[this->mNumSubSamples * 4] = src[4];
                    dest[this->mNumSubSamples * 5] = src[5];
                    dest[this->mNumSubSamples * 6] = src[6];
                    dest[this->mNumSubSamples * 7] = src[7];

                    dest += this->mNumSubSamples * 8;
                    src += 8;
                    count2--;
                }

                count2 = samples & 7;
                while (count2) {
                    dest[0] = src[0];

                    dest += this->mNumSubSamples;
                    src++;
                    count2--;
                }

                break;
            }
            case FMOD_SOUND_FORMAT_PCM24: {
                for (uint32_t j = 0; j < samples; j++) {
                    reinterpret_cast<uint8_t*>(*ptr1)[(((this->mNumSubSamples * j) + i) * 3) + 0] = reinterpret_cast<uint8_t*>(in_ptr1)[(j * 3) + 0];
                    reinterpret_cast<uint8_t*>(*ptr1)[(((this->mNumSubSamples * j) + i) * 3) + 1] = reinterpret_cast<uint8_t*>(in_ptr1)[(j * 3) + 1];
                    reinterpret_cast<uint8_t*>(*ptr1)[(((this->mNumSubSamples * j) + i) * 3) + 2] = reinterpret_cast<uint8_t*>(in_ptr1)[(j * 3) + 2];
                }
                break;
            }
            case FMOD_SOUND_FORMAT_PCM32:
            case FMOD_SOUND_FORMAT_PCMFLOAT: {
                auto dest = reinterpret_cast<uint32_t*>(*ptr1) + i;
                auto src = reinterpret_cast<uint32_t*>(in_ptr1);

                uint32_t count2 = samples >> 3;
                while (count2) {
                    dest[0] = src[0];
                    dest[this->mNumSubSamples * 1] = src[1];
                    dest[this->mNumSubSamples * 2] = src[2];
                    dest[this->mNumSubSamples * 3] = src[3];
                    dest[this->mNumSubSamples * 4] = src[4];
                    dest[this->mNumSubSamples * 5] = src[5];
                    dest[this->mNumSubSamples * 6] = src[6];
                    dest[this->mNumSubSamples * 7] = src[7];

                    dest += this->mNumSubSamples * 8;
                    src += 8;
                    count2--;
                }

                count2 = samples & 7;
                while (count2) {
                    dest[0] = src[0];

                    dest += this->mNumSubSamples;
                    src++;
                    count2--;
                }
                break;
            }
            case FMOD_SOUND_FORMAT_IMAADPCM: {
                if (this->mNumSubSamples == 2) {
                    samples = length / 4;

                    auto dest = reinterpret_cast<uint32_t*>(*ptr1) + i;
                    auto src = reinterpret_cast<uint32_t*>(in_ptr1);

                    uint32_t count2 = samples >> 3;
                    while (count2) {
                        dest[0] = src[0];
                        dest[this->mNumSubSamples * 1] = src[1];
                        dest[this->mNumSubSamples * 2] = src[2];
                        dest[this->mNumSubSamples * 3] = src[3];
                        dest[this->mNumSubSamples * 4] = src[4];
                        dest[this->mNumSubSamples * 5] = src[5];
                        dest[this->mNumSubSamples * 6] = src[6];
                        dest[this->mNumSubSamples * 7] = src[7];

                        dest += this->mNumSubSamples * 8;
                        src += 8;
                        count2--;
                    }

                    count2 = samples & 7;
                    while (count2) {
                        dest[0] = src[0];

                        dest += this->mNumSubSamples;
                        src++;
                        count2--;
                    }
                    break;
                }
            }
            case FMOD_SOUND_FORMAT_VAG: {
                uint32_t blocksize = 0;
                uint32_t blocks;
                auto dest = reinterpret_cast<uint8_t*>(*ptr1);
                auto src = reinterpret_cast<uint8_t*>(in_ptr1);

                SoundI::getBytesFromSamples(1, &blocksize, 1, this->mFormat);

                blocks = length / blocksize;

                dest += (i * blocksize);

                for (uint32_t j = 0; j < blocks; j++) {
                    FMOD_memcpy(dest, src, blocksize);
                    dest += (blocksize * this->mNumSubSamples);
                    src += blocksize;
                }
                break;
            }
            case FMOD_SOUND_FORMAT_XMA: {
                auto dest = reinterpret_cast<int8_t*>(*ptr1) + i;
                auto src = reinterpret_cast<int8_t*>(in_ptr1);

                uint32_t count2 = samples >> 3;
                while (count2) {
                    dest[0] = src[0];
                    dest[this->mNumSubSamples * 1] = src[1];
                    dest[this->mNumSubSamples * 2] = src[2];
                    dest[this->mNumSubSamples * 3] = src[3];
                    dest[this->mNumSubSamples * 4] = src[4];
                    dest[this->mNumSubSamples * 5] = src[5];
                    dest[this->mNumSubSamples * 6] = src[6];
                    dest[this->mNumSubSamples * 7] = src[7];

                    dest += this->mNumSubSamples * 8;
                    src += 8;
                    count2--;
                }

                count2 = samples & 7;
                while (count2) {
                    dest[0] = src[0];

                    dest += this->mNumSubSamples;
                    src++;
                    count2--;
                }
                break;
            }
            default: {
                FMOD_OS_CriticalSection_Leave(this->mSystem->mMultiSubSampleLockBufferCrit);

                return FMOD_ERR_FORMAT;
            }
            }

            this->mSubSample[i]->unlock(in_ptr1, in_ptr2, in_len1, in_len2);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    uint32_t length;
    uint32_t offset;
    uint32_t samples = 0;

    if (!ptr1 || !len1) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mNumSubSamples < 1) {
        return this->unlockInternal(ptr1, ptr2, len1, len2);
    }

    SoundI::getSamplesFromBytes(this->mLockLength, &samples);

    length = this->mLockLength / this->mNumSubSamples;
    offset = this->mLockOffset / this->mNumSubSamples;

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        uint32_t in_len1;
        uint32_t in_len2;
        void* in_ptr1;
        void* in_ptr2;

        this->mSubSample[i]->lock(offset, length, &in_ptr1, &in_ptr2, &in_len1, &in_len2);

        switch (this->mFormat) {
        case FMOD_SOUND_FORMAT_PCM8: {
            auto src = reinterpret_cast<int8_t*>(ptr1) + i;
            auto dest = reinterpret_cast<int8_t*>(in_ptr1);

            uint32_t count2 = samples >> 3;
            while (count2) {
                dest[0] = src[0];
                dest[1] = src[this->mNumSubSamples * 1];
                dest[2] = src[this->mNumSubSamples * 2];
                dest[3] = src[this->mNumSubSamples * 3];
                dest[4] = src[this->mNumSubSamples * 4];
                dest[5] = src[this->mNumSubSamples * 5];
                dest[6] = src[this->mNumSubSamples * 6];
                dest[7] = src[this->mNumSubSamples * 7];

                src += this->mNumSubSamples * 8;
                dest += 8;
                count2--;
            }

            count2 = samples & 7;
            while (count2) {
                dest[0] = src[0];

                src += this->mNumSubSamples;
                dest++;
                count2--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_GCADPCM: {
            samples = length / 2;
        }
        case FMOD_SOUND_FORMAT_PCM16: {
            auto src = reinterpret_cast<int16_t*>(ptr1) + i;
            auto dest = reinterpret_cast<int16_t*>(in_ptr1);

            uint32_t count2 = samples >> 3;
            while (count2) {
                dest[0] = src[0];
                dest[1] = src[this->mNumSubSamples * 1];
                dest[2] = src[this->mNumSubSamples * 2];
                dest[3] = src[this->mNumSubSamples * 3];
                dest[4] = src[this->mNumSubSamples * 4];
                dest[5] = src[this->mNumSubSamples * 5];
                dest[6] = src[this->mNumSubSamples * 6];
                dest[7] = src[this->mNumSubSamples * 7];

                src += this->mNumSubSamples * 8;
                dest += 8;
                count2--;
            }

            count2 = samples & 7;
            while (count2) {
                dest[0] = src[0];

                src += this->mNumSubSamples;
                dest++;
                count2--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM24: {
            for (uint32_t j = 0; j < samples; j++) {
                reinterpret_cast<uint8_t*>(in_ptr1)[(j * 3) + 0] = reinterpret_cast<uint8_t*>(ptr1)[(((this->mNumSubSamples * j) + i) * 3) + 0];
                reinterpret_cast<uint8_t*>(in_ptr1)[(j * 3) + 1] = reinterpret_cast<uint8_t*>(ptr1)[(((this->mNumSubSamples * j) + i) * 3) + 1];
                reinterpret_cast<uint8_t*>(in_ptr1)[(j * 3) + 2] = reinterpret_cast<uint8_t*>(ptr1)[(((this->mNumSubSamples * j) + i) * 3) + 2];
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM32:
        case FMOD_SOUND_FORMAT_PCMFLOAT: {
            auto src = reinterpret_cast<uint32_t*>(ptr1) + i;
            auto dest = reinterpret_cast<uint32_t*>(in_ptr1);

            uint32_t count2 = samples >> 3;
            while (count2) {
                dest[0] = src[0];
                dest[1] = src[this->mNumSubSamples * 1];
                dest[2] = src[this->mNumSubSamples * 2];
                dest[3] = src[this->mNumSubSamples * 3];
                dest[4] = src[this->mNumSubSamples * 4];
                dest[5] = src[this->mNumSubSamples * 5];
                dest[6] = src[this->mNumSubSamples * 6];
                dest[7] = src[this->mNumSubSamples * 7];

                src += this->mNumSubSamples * 8;
                dest += 8;
                count2--;
            }

            count2 = samples & 7;
            while (count2) {
                dest[0] = src[0];

                src += this->mNumSubSamples;
                dest++;
                count2--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_IMAADPCM: {
            if (this->mNumSubSamples == 2) {
                samples = length / 4;

                auto src = reinterpret_cast<uint32_t*>(ptr1) + i;
                auto dest = reinterpret_cast<uint32_t*>(in_ptr1);

                uint32_t count2 = samples >> 3;
                while (count2) {
                    dest[0] = src[0];
                    dest[1] = src[this->mNumSubSamples * 1];
                    dest[2] = src[this->mNumSubSamples * 2];
                    dest[3] = src[this->mNumSubSamples * 3];
                    dest[4] = src[this->mNumSubSamples * 4];
                    dest[5] = src[this->mNumSubSamples * 5];
                    dest[6] = src[this->mNumSubSamples * 6];
                    dest[7] = src[this->mNumSubSamples * 7];

                    src += this->mNumSubSamples * 8;
                    dest += 8;
                    count2--;
                }

                count2 = samples & 7;
                while (count2) {
                    dest[0] = src[0];

                    src += this->mNumSubSamples;
                    dest++;
                    count2--;
                }
                break;
            }
        }
        case FMOD_SOUND_FORMAT_VAG: {
            uint32_t blocks = length / 16;
            auto dest = reinterpret_cast<uint32_t*>(in_ptr1);
            auto src = reinterpret_cast<uint32_t*>(ptr1) + (i * 4);

            for (uint32_t j = 0; j < blocks; j++) {
                dest[0] = src[0];
                dest[1] = src[1];
                dest[2] = src[2];
                dest[3] = src[3];

                dest += 4;
                src += (4 * this->mNumSubSamples);
            }
            break;
        }
        case FMOD_SOUND_FORMAT_XMA: {
            auto src = reinterpret_cast<int8_t*>(ptr1) + i;
            auto dest = reinterpret_cast<int8_t*>(in_ptr1);

            uint32_t count2 = samples >> 3;
            while (count2) {
                dest[0] = src[0];
                dest[1] = src[this->mNumSubSamples * 1];
                dest[2] = src[this->mNumSubSamples * 2];
                dest[3] = src[this->mNumSubSamples * 3];
                dest[4] = src[this->mNumSubSamples * 4];
                dest[5] = src[this->mNumSubSamples * 5];
                dest[6] = src[this->mNumSubSamples * 6];
                dest[7] = src[this->mNumSubSamples * 7];

                src += this->mNumSubSamples * 8;
                dest += 8;
                count2--;
            }

            count2 = samples & 7;
            while (count2) {
                dest[0] = src[0];

                src += this->mNumSubSamples;
                dest++;
                count2--;
            }
            break;
        }
        default: {
            FMOD_OS_CriticalSection_Leave(this->mSystem->mMultiSubSampleLockBufferCrit);

            return FMOD_ERR_FORMAT;
        }
        }

        this->mSubSample[i]->unlock(in_ptr1, in_ptr2, in_len1, in_len2);
    }

    FMOD_OS_CriticalSection_Leave(this->mSystem->mMultiSubSampleLockBufferCrit);

    return FMOD_OK;
}

FMOD_RESULT Sample::setDefaults(
    float frequency,
    float volume,
    float pan,
    int32_t priority) {

    FMOD_RESULT result;

    result = SoundI::setDefaults(frequency, volume, pan, priority);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        this->mSubSample[i]->setDefaults(frequency, volume, pan, priority);
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::setVariations(float frequencyvar, float volumevar, float panvar) {
    FMOD_RESULT result;

    result = SoundI::setVariations(frequencyvar, volumevar, panvar);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        this->mSubSample[i]->setVariations(frequencyvar, volumevar, panvar);
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::set3DMinMaxDistance(float min, float max) {
    FMOD_RESULT result;

    result = SoundI::set3DMinMaxDistance(min, max);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        this->mSubSample[i]->set3DMinMaxDistance(min, max);
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::set3DConeSettings(
    float insideconeangle,
    float outsideconeangle,
    float outsidevolume) {
    FMOD_RESULT result;

    result = SoundI::set3DConeSettings(
        insideconeangle,
        outsideconeangle,
        outsidevolume);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        this->mSubSample[i]->set3DConeSettings(
            insideconeangle,
            outsideconeangle,
            outsidevolume);
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::setMode(FMOD_MODE mode) {
    FMOD_RESULT result;

    result = SoundI::setMode(mode);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        this->mSubSample[i]->setMode(mode);
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::setLoopCount(int32_t loopcount) {
    FMOD_RESULT result;

    result = SoundI::setLoopCount(loopcount);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        this->mSubSample[i]->setLoopCount(loopcount);
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::setLoopPoints(
    uint32_t loopstart,
    FMOD_TIMEUNIT loopstarttype,
    uint32_t loopend,
    FMOD_TIMEUNIT loopendtype) {
    FMOD_RESULT result;

    result = SoundI::setLoopPoints(
        loopstart, loopstarttype,
        loopend, loopendtype);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumSubSamples; i++) {
        this->mSubSample[i]->setLoopPoints(
            loopstart, loopstarttype,
            loopend, loopendtype);
    }

    return FMOD_OK;
}

FMOD_RESULT Sample::lockInternal(uint32_t offset,
                                 uint32_t length,
                                 void** ptr1,
                                 void** ptr2,
                                 uint32_t* len1,
                                 uint32_t* len2) {
    return FMOD_ERR_SUBSOUNDS;
}

FMOD_RESULT Sample::unlockInternal(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    return FMOD_OK;
}

FMOD_RESULT Sample::setBufferData(void* data) {
    return FMOD_ERR_NEEDSSOFTWARE;
}

} // namespace FMOD
