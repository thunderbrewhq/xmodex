#include "xmodex/dsp/DSPFilter.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/DSPConnection.hpp"

namespace FMOD {

DSPFilter::DSPFilter() {
}

FMOD_RESULT DSPFilter::execute(float* inbuffer, float** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode) {
    FMOD_RESULT result = FMOD_OK;
    LinkedListNode* current;
    float* mixbuffer;
    float* tempbuffer;
    int32_t tempbufferindex;
    int32_t count;

    if (*length > this->mSystem->mDSPBlockSize) {
        *length = this->mSystem->mDSPBlockSize;
    }

    *outbuffer = inbuffer;
    *outchannels = inchannels;

    this->mFlags |= FMOD_DSP_FLAG_IDLE;

    if (this->mFlags & FMOD_DSP_FLAG_VISITED) {
        *outbuffer = this->mBuffer;
        *outchannels = this->mBufferChannels;

        this->mFlags &= ~FMOD_DSP_FLAG_IDLE;

        return result;
    }

    if (this->mBuffer) {
        mixbuffer = this->mBuffer;
        this->mBufferChannels = inchannels;
    } else {
        mixbuffer = inbuffer;
    }

    tempbufferindex = this->mSystem->mDSPTempBuffIndex;
    tempbuffer = this->mSystem->mDSPTempBuff[tempbufferindex];

    count = 0;
    current = this->mInputHead.getNext();

    if (current == &this->mInputHead) {
        FMOD_memset(mixbuffer, 0, *length * inchannels * sizeof(float));
    }

    while (current != &this->mInputHead) {
        auto connection = static_cast<DSPConnection*>(current->getData());
        auto input = connection->mInputUnit;

        if ((input->mFlags & FMOD_DSP_FLAG_ACTIVE) && !(input->mFlags & FMOD_DSP_FLAG_FINISHED)) {
            int32_t nextbufferindex = 1 - tempbufferindex;

            this->mSystem->mDSPTempBuffIndex = nextbufferindex;

            result = input->execute(tempbuffer, outbuffer, length, inchannels, outchannels, speakermode);
            if (result != FMOD_OK) {
                break;
            }

            if (!(input->mFlags & FMOD_DSP_FLAG_IDLE)) {
                bool unity = false;

                this->mFlags &= ~FMOD_DSP_FLAG_IDLE;

                if (this->mNumInputs <= 1 && connection->mVolume == 1.0f && ((!connection->mSetLevelsUsed && this->mDescription.mCategory != FMOD_DSP_CATEGORY_SOUNDCARD) || connection->checkUnity(*outchannels, inchannels) == FMOD_OK)) {
                    unity = true;
                }

                if (unity && (!this->mDescription.read || !this->mDescription.channels || this->mDescription.channels == *outchannels)) {
                    if (this->mNumOutputs > 1) {
                        if (*outbuffer) {
                            FMOD_memcpy(this->mBuffer, *outbuffer, *length * *outchannels * sizeof(float));
                        } else {
                            FMOD_memset(this->mBuffer, 0, *length * *outchannels * sizeof(float));
                        }

                        *outbuffer = this->mBuffer;
                        this->mFlags &= ~FMOD_DSP_FLAG_IDLE;
                        this->mBufferChannels = *outchannels;
                    }

                    input->mFlags |= FMOD_DSP_FLAG_VISITED;
                    break;
                }

                if (!connection->mSetLevelsUsed) {
                    if (inchannels == *outchannels && connection->mVolume == 1.0f) {
                        connection->setUnity();
                    } else {
                        connection->setPan(0.0f, inchannels, *outchannels, speakermode);
                    }

                    connection->mSetLevelsUsed = true;
                }

                if (*outbuffer == mixbuffer) {
                    if (mixbuffer == this->mSystem->mDSPTempBuff[0]) {
                        mixbuffer = this->mSystem->mDSPTempBuff[1];
                    } else {
                        mixbuffer = this->mSystem->mDSPTempBuff[0];
                    }

                    tempbufferindex = 1 - tempbufferindex;
                    tempbuffer = this->mSystem->mDSPTempBuff[nextbufferindex];
                }

                if (!count) {
                    FMOD_memset(mixbuffer, 0, *length * inchannels * sizeof(float));
                }

                connection->mix(mixbuffer, *outbuffer, inchannels, *outchannels, *length);
            } else if (!count) {
                FMOD_memset(mixbuffer, 0, *length * inchannels * sizeof(float));
            }
        } else if (!count) {
            FMOD_memset(mixbuffer, 0, *length * inchannels * sizeof(float));
        }

        *outbuffer = mixbuffer;
        *outchannels = inchannels;

        input->mFlags |= FMOD_DSP_FLAG_VISITED;

        current = current->getNext();
        count++;
    }

    if (this->mDescription.read && !(this->mFlags & FMOD_DSP_FLAG_BYPASS)) {
        int32_t channels = *outchannels;

        if (this->mDescription.channels) {
            *outchannels = this->mDescription.channels;
            FMOD_memset(inbuffer, 0, *length * this->mDescription.channels * sizeof(float));
        } else if (!this->mNumInputs) {
            FMOD_memset(*outbuffer, 0, *length * channels * sizeof(float));
        }

        this->instance = reinterpret_cast<FMOD_DSP*>(this);

        this->mDescription.read(static_cast<FMOD_DSP_STATE*>(this), *outbuffer, inbuffer, *length, channels, *outchannels);

        *outbuffer = inbuffer;

        this->mFlags &= ~FMOD_DSP_FLAG_IDLE;
    }

    if (this->mHistoryBuffer) {
        auto len = *length;
        auto src = *outbuffer;

        while (len) {
            auto size = len;

            if (this->mHistoryPosition + size > this->mHistoryLength) {
                size = this->mHistoryLength - this->mHistoryPosition;
            }

            FMOD_memcpy(this->mHistoryBuffer + (this->mHistoryPosition * *outchannels), src, size * *outchannels * sizeof(float));

            this->mHistoryPosition += size;
            src += size * *outchannels;
            len -= size;

            if (this->mHistoryPosition >= this->mHistoryLength) {
                this->mHistoryPosition = 0;
            }
        }
    }

    return result;
}

FMOD_RESULT DSPFilter::startBuffering(uint32_t historylength) {
    int32_t channels;

    if (!this->mHistoryBuffer || historylength != this->mHistoryLength) {
        this->mSystem->getSoftwareFormat(nullptr, nullptr, &channels, nullptr, nullptr, nullptr);

        if (this->mHistoryBuffer) {
            FMOD_Memory_Free(this->mHistoryBuffer);
        }

        this->mHistoryLength = historylength;

        this->mHistoryBuffer = static_cast<float*>(FMOD_Memory_Calloc(historylength * channels * sizeof(float)));
        if (!this->mHistoryBuffer) {
            return FMOD_ERR_MEMORY;
        }

        this->mHistoryPosition = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFilter::stopBuffering() {
    if (this->mHistoryBuffer) {
        FMOD_Memory_Free(this->mHistoryBuffer);
        this->mHistoryBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFilter::getHistoryBuffer(float** buffer, uint32_t* position, uint32_t* length) {
    if (buffer) {
        *buffer = this->mHistoryBuffer;
    }

    if (position) {
        *position = this->mHistoryPosition;
    }

    if (length) {
        *length = this->mHistoryLength;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFilter::release(bool freethis) {
    this->stopBuffering();

    return DSPI::release(freethis);
}

} // namespace FMOD
