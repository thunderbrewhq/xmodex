#include "xmodex/sound/SampleSoftware.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/DSPResampler.hpp"
#include "xmodex/os/Time.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

SampleSoftware::SampleSoftware() {
    this->mBuffer = nullptr;
    this->mBufferMemory = nullptr;
    this->mDataEndCopied = false;
}

FMOD_RESULT SampleSoftware::release(bool freethis) {
    FMOD_RESULT result;

    if (!this->mSystem) {
        return FMOD_ERR_UNINITIALIZED;
    }

    while ((this->mOpenState != FMOD_OPENSTATE_READY && this->mOpenState != FMOD_OPENSTATE_ERROR) || this->mFlags & FMOD_SOUND_FLAG_DONOTRELEASE) {
        FMOD_OS_Time_Sleep(2);
    }

    result = this->mSystem->stopSound(this);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mBufferMemory) {
        FMOD_Memory_Free(this->mBufferMemory);
        this->mBufferMemory = nullptr;
    }

    if (this->mLoopPointDataEnd && this->mLoopPointDataEnd != this->mLoopPointDataEndMemory) {
        FMOD_Memory_Free(this->mLoopPointDataEnd);
        this->mLoopPointDataEnd = nullptr;
    }

    this->mBuffer = nullptr;

    return Sample::release(freethis);
}

FMOD_RESULT SampleSoftware::restoreLoopPointData() {
    FMOD_RESULT result;
    uint32_t overflowbytes;
    uint32_t pointA;
    uint32_t pointB;

    if ((this->mFormat != FMOD_SOUND_FORMAT_PCM8 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCM16 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCM24 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCM32 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCMFLOAT) ||
        !this->mLoopPointDataEnd) {
        return FMOD_OK;
    }

    result = this->getBytesFromSamples(FMOD_DSP_RESAMPLER_OVERFLOWLENGTH, &overflowbytes);
    if (result != FMOD_OK) {
        return result;
    }
    result = this->getBytesFromSamples(this->mLoopStart, &pointA);
    if (result != FMOD_OK) {
        return result;
    }
    result = this->getBytesFromSamples(this->mLoopStart + this->mLoopLength, &pointB);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mDataEndCopied) {
        FMOD_memcpy(reinterpret_cast<char*>(this->mBuffer) + pointB, this->mLoopPointDataEnd, overflowbytes);
        this->mDataEndCopied = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT SampleSoftware::lockInternal(
    uint32_t offset,
    uint32_t length,
    void** ptr1,
    void** ptr2,
    uint32_t* len1,
    uint32_t* len2) {

    FMOD_RESULT result;
    char* src = nullptr;
    uint32_t overflowbytes;
    uint32_t pointB;

    result = this->getBytesFromSamples(FMOD_DSP_RESAMPLER_OVERFLOWLENGTH, &overflowbytes);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->getBytesFromSamples(this->mLoopStart + this->mLoopLength, &pointB);
    if (result != FMOD_OK) {
        return result;
    }

    if (offset >= pointB && offset < pointB + overflowbytes) {
        result = this->restoreLoopPointData();
        if (result != FMOD_OK) {
            return result;
        }
    }

    src = reinterpret_cast<char*>(this->mBuffer);

    if (offset >= this->mLengthBytes || offset < 0 || length < 0 || length > this->mLengthBytes) {
        *ptr1 = nullptr;

        if (ptr2) {
            *ptr2 = nullptr;
        }

        *len1 = 0;

        if (len2) {
            *len2 = 0;
        }
        return FMOD_ERR_INVALID_PARAM;
    }

    if (offset + length <= this->mLengthBytes) {
        *ptr1 = src + offset;
        *len1 = length;

        if (ptr2) {
            *ptr2 = nullptr;
        }

        if (len2) {
            *len2 = 0;
        }
    } else {
        *ptr1 = src + offset;
        *len1 = this->mLengthBytes - offset;

        if (ptr2) {
            *ptr2 = src;
        }

        if (len2) {
            *len2 = length - (this->mLengthBytes - offset);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SampleSoftware::unlockInternal(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    return this->setLoopPointData();
}

FMOD_RESULT SampleSoftware::setBufferData(void* data) {
    this->mBuffer = data;
    return FMOD_OK;
}

FMOD_RESULT SampleSoftware::setLoopPointData() {
    FMOD_RESULT result;
    uint32_t overflowbytes;
    uint32_t pointA;
    uint32_t pointB;

    if ((this->mFormat != FMOD_SOUND_FORMAT_PCM8 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCM16 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCM24 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCM32 &&
         this->mFormat != FMOD_SOUND_FORMAT_PCMFLOAT) ||
        !this->mLoopPointDataEnd) {
        return FMOD_OK;
    }

    result = this->getBytesFromSamples(FMOD_DSP_RESAMPLER_OVERFLOWLENGTH, &overflowbytes);
    if (result != FMOD_OK) {
        return result;
    }
    result = this->getBytesFromSamples(this->mLoopStart, &pointA);
    if (result != FMOD_OK) {
        return result;
    }
    result = this->getBytesFromSamples(this->mLoopStart + this->mLoopLength, &pointB);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mMode & FMOD_LOOP_BIDI) {
        FMOD_memcpy(this->mLoopPointDataEnd, reinterpret_cast<char*>(this->mBuffer) + pointB, overflowbytes);
        this->mDataEndCopied = true;

        switch (this->mFormat) {
        case FMOD_SOUND_FORMAT_PCM8: {
            char* srcptr;
            char* destptr;

            srcptr = destptr = reinterpret_cast<char*>(this->mBuffer) + pointB;
            srcptr -= this->mChannels;
            for (int32_t i = 0; i < FMOD_DSP_RESAMPLER_OVERFLOWLENGTH * this->mChannels; i++) {
                *destptr = *srcptr;
                destptr++;
                srcptr--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM16: {
            int16_t* srcptr;
            int16_t* destptr;

            srcptr = destptr = reinterpret_cast<int16_t*>(reinterpret_cast<char*>(this->mBuffer) + pointB);
            srcptr -= this->mChannels;
            for (int32_t i = 0; i < FMOD_DSP_RESAMPLER_OVERFLOWLENGTH * this->mChannels; i++) {
                *destptr = *srcptr;
                destptr++;
                srcptr--;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM24: {
            char* srcptr;
            char* destptr;

            srcptr = destptr = reinterpret_cast<char*>(this->mBuffer) + pointB;
            srcptr -= (this->mChannels * 3);
            for (int32_t i = 0; i < FMOD_DSP_RESAMPLER_OVERFLOWLENGTH * this->mChannels; i++) {
                destptr[0] = srcptr[0];
                destptr[1] = srcptr[1];
                destptr[2] = srcptr[2];
                destptr += 3;
                srcptr -= 3;
            }
            break;
        }
        case FMOD_SOUND_FORMAT_PCM32:
        case FMOD_SOUND_FORMAT_PCMFLOAT: {
            int32_t* srcptr;
            int32_t* destptr;

            srcptr = destptr = reinterpret_cast<int32_t*>(reinterpret_cast<char*>(this->mBuffer) + pointB);
            srcptr -= this->mChannels;
            for (int32_t i = 0; i < FMOD_DSP_RESAMPLER_OVERFLOWLENGTH * this->mChannels; i++) {
                *destptr = *srcptr;
                destptr++;
                srcptr--;
            }
            break;
        }
        default: {
            break;
        }
        }
    } else if (this->mMode & FMOD_LOOP_NORMAL) {
        FMOD_memcpy(this->mLoopPointDataEnd, reinterpret_cast<char*>(this->mBuffer) + pointB, overflowbytes);
        this->mDataEndCopied = true;

        FMOD_memcpy(reinterpret_cast<char*>(this->mBuffer) + pointB, reinterpret_cast<char*>(this->mBuffer) + pointA, overflowbytes);
    } else if (this->mMode & FMOD_LOOP_OFF) {
        if (this->mDataEndCopied) {
            FMOD_memcpy(reinterpret_cast<char*>(this->mBuffer) + pointB, this->mLoopPointDataEnd, overflowbytes);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SampleSoftware::setMode(FMOD_MODE mode) {
    FMOD_RESULT result;

    result = Sample::setMode(mode);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->setLoopPointData();
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

} // namespace FMOD
