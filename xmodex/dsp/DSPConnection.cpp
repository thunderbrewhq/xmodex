#include "xmodex/dsp/DSPConnection.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/dsp/DSPI.hpp"

namespace FMOD {

FMOD_RESULT DSPConnection::init(float*& levelmemory, int32_t maxoutputlevels, int32_t maxinputlevels) {
    this->mMaxOutputLevels = maxoutputlevels;
    this->mMaxInputLevels = maxinputlevels;

    if (maxoutputlevels < 2) {
        maxoutputlevels = 2;
    }

    if (this->mMaxInputLevels < this->mMaxOutputLevels) {
        this->mMaxInputLevels = this->mMaxOutputLevels;
    }

    for (int32_t i = 0; i < DSP_MAXLEVELS_OUT; i++) {
        if (i < mMaxOutputLevels) {
            this->mLevel[i] = levelmemory;
            levelmemory += this->mMaxInputLevels;
            this->mLevelCurrent[i] = levelmemory;
            levelmemory += this->mMaxInputLevels;
            this->mLevelDelta[i] = levelmemory;
            levelmemory += this->mMaxInputLevels;
        } else {
            this->mLevel[i] = nullptr;
            this->mLevelCurrent[i] = nullptr;
            this->mLevelDelta[i] = nullptr;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::reset() {
    this->mVolume = 1.0f;

    for (int32_t i = 0; i < this->mMaxOutputLevels; i++) {
        for (int32_t j = 0; j < this->mMaxInputLevels; j++) {
            this->mLevel[i][j] = 0.0f;
            this->mLevelCurrent[i][j] = 0.0f;
            this->mLevelDelta[i][j] = 0.0f;
        }
    }

    this->mSetLevelsUsed = 0;

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::setUnity() {
    for (int32_t i = 0; i < this->mMaxOutputLevels; i++) {
        for (int32_t j = 0; j < this->mMaxInputLevels; j++) {
            if (i == j) {
                this->mLevelCurrent[i][j] = 1.0f;
                this->mLevel[i][j] = 1.0f;
            } else {
                this->mLevel[i][j] = 0.0f;
                this->mLevelCurrent[i][j] = 0.0f;
            }
        }
    }

    this->mVolume = 1.0f;
    this->mRampCount = 0;

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::mixAndRamp(float* outbuffer, float* inbuffer, int32_t outchannels, int32_t inchannels, uint32_t length) {
    for (int32_t i = 0; i < length; i++) {
        for (int32_t j = 0; j < outchannels; j++) {
            float srcval = 0.0f;
            float destval = *outbuffer;

            for (int32_t k = 0; k < inchannels; k++) {
                srcval += (inbuffer[k] * this->mLevelCurrent[j][k]);
                this->mLevelCurrent[j][k] += this->mLevelDelta[j][k];
            }

            destval += srcval;
            *outbuffer++ = destval;
        }

        inbuffer += inchannels;
    }

    this->mRampCount -= length;

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::rampTo() {
    float oorampcount;
    float delta = 0.0f;

    oorampcount = (1.0f / DSP_RAMPCOUNT);

    if (this->mMaxInputLevels == 6) {
        for (int32_t i = 0; i < this->mMaxOutputLevels; i++) {
            float value;

            value = ((this->mLevel[i][0] * this->mVolume) - this->mLevelCurrent[i][0]) * oorampcount;
            delta += FMOD_FABS(value);
            this->mLevelDelta[i][0] = value;
            value = ((this->mLevel[i][1] * this->mVolume) - this->mLevelCurrent[i][1]) * oorampcount;
            delta += FMOD_FABS(value);
            this->mLevelDelta[i][1] = value;
            value = ((this->mLevel[i][2] * this->mVolume) - this->mLevelCurrent[i][2]) * oorampcount;
            delta += FMOD_FABS(value);
            this->mLevelDelta[i][2] = value;
            value = ((this->mLevel[i][3] * this->mVolume) - this->mLevelCurrent[i][3]) * oorampcount;
            delta += FMOD_FABS(value);
            this->mLevelDelta[i][3] = value;
            value = ((this->mLevel[i][4] * this->mVolume) - this->mLevelCurrent[i][4]) * oorampcount;
            delta += FMOD_FABS(value);
            this->mLevelDelta[i][4] = value;
            value = ((this->mLevel[i][5] * this->mVolume) - this->mLevelCurrent[i][5]) * oorampcount;
            delta += FMOD_FABS(value);
            this->mLevelDelta[i][5] = value;
        }
    } else {
        for (int32_t i = 0; i < this->mMaxOutputLevels; i++) {
            for (int32_t j = 0; j < this->mMaxInputLevels; j++) {
                float value;

                value = ((this->mLevel[i][j] * this->mVolume) - this->mLevelCurrent[i][j]) * oorampcount;
                delta += FMOD_FABS(value);
                this->mLevelDelta[i][j] = value;
            }
        }
    }

    if (delta > 0.000001f) {
        this->mRampCount = DSP_RAMPCOUNT;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::checkUnity(int32_t outchannels, int32_t inchannels) {
    if (this->mVolume != 1.0f || inchannels != outchannels) {
        return FMOD_ERR_PAN;
    }

    for (int32_t i = 0; i < outchannels; i++) {
        for (int32_t j = 0; j < inchannels; j++) {
            if (i == j && this->mLevel[i][j] != 1.0f) {
                return FMOD_ERR_PAN;
            } else if (i != j && this->mLevel[i][j] != 0.0f) {
                return FMOD_ERR_PAN;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::setPan(float pan, int32_t outchannels, int32_t inchannels, FMOD_SPEAKERMODE speakermode) {
    if (!this->mInputUnit->mSystem) {
        return FMOD_ERR_INTERNAL;
    }

    for (int32_t i = 0; i < outchannels; i++) {
        for (int32_t j = 0; j < inchannels; j++) {
            this->mLevel[i][j] = 0.0f;
        }
    }

    switch (speakermode) {
    case FMOD_SPEAKERMODE_RAW: {
        for (int32_t i = 0; i < outchannels; i++) {
            for (int32_t j = 0; j < inchannels; j++) {
                if (i == j) {
                    this->mLevel[i][j] = 1.0f;
                }
            }
        }
        break;
    }
    case FMOD_SPEAKERMODE_MONO: {
        for (int32_t i = 0; i < inchannels; i++) {
            this->mLevel[0][i] = 1.0f;
        }
        break;
    }
    case FMOD_SPEAKERMODE_STEREO:
    case FMOD_SPEAKERMODE_STEREO_LINEAR: {
        float l;
        float r;

        pan = (pan + 1.0f) / 2.0f;

        if (inchannels == 1) {
            l = 1.0f - pan;
            r = pan;

            if (speakermode == FMOD_SPEAKERMODE_STEREO) {
                l = FMOD_SQRT(l);
                r = FMOD_SQRT(r);
            }

            this->mLevel[0][0] = l;
            this->mLevel[1][0] = r;
        } else if (inchannels == 2 || inchannels == 4 || inchannels == 6 || inchannels == 8) {
            if (pan <= 0.5f) {
                l = 1.0f;
                r = pan * 2.0f;
            } else {
                l = (1.0f - pan) * 2.0f;
                r = 1.0f;
            }

            if (inchannels == 2) {
                this->mLevel[0][0] = l;
                this->mLevel[1][1] = r;
            } else if (inchannels == 4) {
                this->mLevel[0][0] = 1.0f * l;
                this->mLevel[0][1] = 0.0f * l;
                this->mLevel[0][2] = 0.707f * l;
                this->mLevel[0][3] = -0.707f * l;
                this->mLevel[1][0] = 0.0f * r;
                this->mLevel[1][1] = 1.0f * r;
                this->mLevel[1][2] = 0.707f * r;
                this->mLevel[1][3] = -0.707f * r;
            } else if (inchannels == 6) {
                this->mLevel[0][0] = 1.0f * l;
                this->mLevel[0][1] = 0.0f * l;
                this->mLevel[0][2] = 0.707f * l;
                this->mLevel[0][3] = 0.0f * l;
                this->mLevel[0][4] = -0.872f * l;
                this->mLevel[0][5] = -0.49f * l;
                this->mLevel[1][0] = 0.0f * r;
                this->mLevel[1][1] = 1.0f * r;
                this->mLevel[1][2] = 0.707f * r;
                this->mLevel[1][3] = 0.0f * r;
                this->mLevel[1][4] = 0.49f * r;
                this->mLevel[1][5] = 0.872f * r;
            } else if (inchannels == 8) {
                this->mLevel[0][0] = 1.0f * l;
                this->mLevel[0][1] = 0.0f * l;
                this->mLevel[0][2] = 0.707f * l;
                this->mLevel[0][3] = 0.0f * l;
                this->mLevel[0][4] = -0.872f * l;
                this->mLevel[0][5] = -0.49f * l;
                this->mLevel[0][6] = 1.0f * l;
                this->mLevel[0][7] = 0.0f * l;
                this->mLevel[1][0] = 0.0f * r;
                this->mLevel[1][1] = 1.0f * r;
                this->mLevel[1][2] = 0.707f * r;
                this->mLevel[1][3] = 0.0f * r;
                this->mLevel[1][4] = 0.49f * r;
                this->mLevel[1][5] = 0.872f * r;
                this->mLevel[1][6] = 0.0f * r;
                this->mLevel[1][7] = 1.0f * r;
            }
        } else {
            for (int32_t i = 0; i < outchannels; i++) {
                for (int32_t j = 0; j < inchannels; j++) {
                    if (i == j) {
                        this->mLevel[i][j] = 1.0f;
                    }
                }
            }
        }
        break;
    }
    case FMOD_SPEAKERMODE_QUAD:
    case FMOD_SPEAKERMODE_SURROUND:
    case FMOD_SPEAKERMODE_5POINT1:
    case FMOD_SPEAKERMODE_7POINT1: {
        float l;
        float r;

        pan = (pan + 1.0f) / 2.0f;

        if (inchannels == 1) {
            l = 1.0f - pan;
            r = pan;

            l = FMOD_SQRT(l);
            r = FMOD_SQRT(r);

            this->mLevel[0][0] = l;
            this->mLevel[1][0] = r;
        } else if (inchannels == 2) {
            if (pan <= 0.5f) {
                l = 1.0f;
                r = pan * 2.0f;
            } else {
                l = (1.0f - pan) * 2.0f;
                r = 1.0f;
            }

            this->mLevel[0][0] = l;
            this->mLevel[1][1] = r;
        } else {
            for (int32_t i = 0; i < outchannels; i++) {
                for (int32_t j = 0; j < inchannels; j++) {
                    if (i == j) {
                        this->mLevel[i][j] = 1.0f;
                    }
                }
            }
        }
        break;
    }
    case FMOD_SPEAKERMODE_PROLOGIC: {
        float l;
        float r;

        pan = (pan + 1.0f) / 2.0f;

        if (inchannels == 1) {
            l = 1.0f - pan;
            r = pan;

            l = FMOD_SQRT(l);
            r = FMOD_SQRT(r);

            this->mLevel[0][0] = l;
            this->mLevel[1][0] = r;
        } else if (inchannels == 2) {
            if (pan < 0.5f) {
                l = 1.0f;
                r = pan * 2.0f;
            } else {
                l = (1.0f - pan) * 2.0f;
                r = 1.0f;
            }

            this->mLevel[0][0] = l;
            this->mLevel[1][1] = r;
        } else {
            for (int32_t i = 0; i < outchannels; i++) {
                for (int32_t j = 0; j < inchannels; j++) {
                    if (i == j) {
                        this->mLevel[i][j] = 1.0f;
                    }
                }
            }
        }
        break;
    }
    default: {
        break;
    }
    }

    this->mSetLevelsUsed = true;

    return this->rampTo();
}

FMOD_RESULT DSPConnection::setMix(float volume) {
    if (volume < -1.0f) {
        volume = -1.0f;
    }
    if (volume > 1.0f) {
        volume = 1.0f;
    }

    if (this->mVolume == volume) {
        return FMOD_OK;
    }

    this->mVolume = volume;

    return this->rampTo();
}

FMOD_RESULT DSPConnection::setLevels(float* levels, int32_t numinputlevels) {
    if (!levels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!numinputlevels) {
        return FMOD_OK;
    }

    if (this->mMaxOutputLevels == 6 && numinputlevels < 3) {
        if (numinputlevels == 1) {
            this->mLevel[0][0] = levels[0];
            this->mLevel[1][0] = levels[1];
            this->mLevel[2][0] = levels[2];
            this->mLevel[3][0] = levels[3];
            this->mLevel[4][0] = levels[4];
            this->mLevel[5][0] = levels[5];
        } else {
            this->mLevel[0][0] = levels[0];
            this->mLevel[0][1] = levels[1];
            this->mLevel[1][0] = levels[2];
            this->mLevel[1][1] = levels[3];
            this->mLevel[2][0] = levels[4];
            this->mLevel[2][1] = levels[5];
            this->mLevel[3][0] = levels[6];
            this->mLevel[3][1] = levels[7];
            this->mLevel[4][0] = levels[8];
            this->mLevel[4][1] = levels[9];
            this->mLevel[5][0] = levels[10];
            this->mLevel[5][1] = levels[11];
        }
    } else {
        for (int32_t i = 0; i < this->mMaxOutputLevels; i++) {
            for (int32_t j = 0; j < this->mMaxInputLevels; j++) {
                if (j < numinputlevels) {
                    this->mLevel[i][j] = levels[(i * numinputlevels) + j];
                } else {
                    this->mLevel[i][j] = 0.0f;
                }
            }
        }
    }

    this->mSetLevelsUsed = 1;

    return this->rampTo();
}

FMOD_RESULT DSPConnection::getLevels(float* levels, int32_t numinputlevels) {
    if (!levels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    for (int32_t i = 0; i < this->mMaxOutputLevels; i++) {
        for (int32_t j = 0; j < numinputlevels; j++) {
            if (j < this->mMaxInputLevels) {
                levels[(i * numinputlevels) + j] = this->mLevel[i][j];
            } else {
                levels[(i * numinputlevels) + j] = 0.0f;
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::copy(DSPConnection* source) {
    this->mVolume = source->mVolume;

    for (int32_t i = 0; i < this->mMaxOutputLevels; i++) {
        for (int32_t j = 0; j < this->mMaxInputLevels; j++) {
            this->mLevel[i][j] = source->mLevel[i][j];
            this->mLevelCurrent[i][j] = source->mLevelCurrent[i][j];
            this->mLevelDelta[i][j] = source->mLevelDelta[i][j];
        }
    }

    this->mRampCount = source->mRampCount;
    this->mSetLevelsUsed = source->mSetLevelsUsed;

    return FMOD_OK;
}

FMOD_RESULT DSPConnection::mix(float* outbuffer, float* inbuffer, int32_t outchannels, int32_t inchannels, uint32_t length) {
    float l00;
    float l10;
    float l01;
    float l11;
    float l20;
    float l30;
    float l40;
    float l50;
    float l21;
    float l31;
    float l41;
    float l51;
    float l22;
    float l33;
    float l44;
    float l55;
    float l60;
    float l70;
    float l61;
    float l71;
    float l66;
    float l77;
    bool supportssimd;

    if (!inbuffer) {
        return FMOD_OK;
    }

    // TODO:
    //
    // supportssimd = FMOD_OS_SupportsSIMD();

    supportssimd = false;

    if (this->mRampCount) {
        FMOD_RESULT result;
        auto len = this->mRampCount;

        if (len > length) {
            len = length;
        }

        result = this->mixAndRamp(outbuffer, inbuffer, outchannels, inchannels, len);
        if (result != FMOD_OK) {
            return result;
        }

        length -= len;
        outbuffer += (len * outchannels);
        inbuffer += (len * inchannels);
    }

    if (!length) {
        return FMOD_OK;
    }

    if (this->mVolume == 0.0f) {
        return FMOD_OK;
    }

    l00 = this->mLevelCurrent[0] ? this->mLevelCurrent[0][0] : 0.0f;
    l10 = this->mLevelCurrent[1] ? this->mLevelCurrent[1][0] : 0.0f;
    l20 = this->mLevelCurrent[2] ? this->mLevelCurrent[2][0] : 0.0f;
    l30 = this->mLevelCurrent[3] ? this->mLevelCurrent[3][0] : 0.0f;
    l40 = this->mLevelCurrent[4] ? this->mLevelCurrent[4][0] : 0.0f;
    l50 = this->mLevelCurrent[5] ? this->mLevelCurrent[5][0] : 0.0f;
    l60 = this->mLevelCurrent[6] ? this->mLevelCurrent[6][0] : 0.0f;
    l70 = this->mLevelCurrent[7] ? this->mLevelCurrent[7][0] : 0.0f;
    l01 = this->mLevelCurrent[0] ? this->mLevelCurrent[0][1] : 0.0f;
    l11 = this->mLevelCurrent[1] ? this->mLevelCurrent[1][1] : 0.0f;
    l21 = this->mLevelCurrent[2] ? this->mLevelCurrent[2][1] : 0.0f;
    l31 = this->mLevelCurrent[3] ? this->mLevelCurrent[3][1] : 0.0f;
    l41 = this->mLevelCurrent[4] ? this->mLevelCurrent[4][1] : 0.0f;
    l51 = this->mLevelCurrent[5] ? this->mLevelCurrent[5][1] : 0.0f;
    l22 = this->mLevelCurrent[2] ? this->mLevelCurrent[2][2] : 0.0f;
    l33 = this->mLevelCurrent[3] ? this->mLevelCurrent[3][3] : 0.0f;
    l44 = this->mLevelCurrent[4] ? this->mLevelCurrent[4][4] : 0.0f;
    l55 = this->mLevelCurrent[5] ? this->mLevelCurrent[5][5] : 0.0f;
    l61 = this->mLevelCurrent[6] ? this->mLevelCurrent[6][1] : 0.0f;
    l71 = this->mLevelCurrent[7] ? this->mLevelCurrent[7][1] : 0.0f;
    l66 = this->mLevelCurrent[6] ? this->mLevelCurrent[6][6] : 0.0f;
    l77 = this->mLevelCurrent[7] ? this->mLevelCurrent[7][7] : 0.0f;

    if (outchannels == 2 && (inchannels == 1 || inchannels == 2)) {
        if (inchannels == 1) {
            if (supportssimd) {
                // TODO:
                //
                // while (reinterpret_cast<uintptr_t>(outbuffer) & 0xF && length) {
                //     outbuffer[0] += (inbuffer[0] * l00);
                //     outbuffer[1] += (inbuffer[0] * l10);
                //     inbuffer++;
                //     outbuffer += 2;
                //     length--;
                // }

                // FMOD_DSP_Connection_MixMonoToStereo_SIMD(inbuffer, outbuffer, length, l00, l10);
            } else {
                uint32_t len;

                len = length >> 2;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[2] += (inbuffer[1] * l00);
                    outbuffer[3] += (inbuffer[1] * l10);
                    outbuffer[4] += (inbuffer[2] * l00);
                    outbuffer[5] += (inbuffer[2] * l10);
                    outbuffer[6] += (inbuffer[3] * l00);
                    outbuffer[7] += (inbuffer[3] * l10);
                    inbuffer += 4;
                    outbuffer += 8;
                    len--;
                }
                len = length & 3;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[1] += (inbuffer[0] * l10);
                    inbuffer++;
                    outbuffer += 2;
                    len--;
                }
            }
        } else if (inchannels == 2) {
            uint32_t len;

            if (l01 < DSP_LEVEL_SMALLVAL && l10 < DSP_LEVEL_SMALLVAL) {
                if (supportssimd) {
                    // TODO:
                    //
                    // while (reinterpret_cast<uintptr_t>(outbuffer) & 0xF && length) {
                    //     outbuffer[0] += (inbuffer[0] * l00);
                    //     outbuffer[1] += (inbuffer[1] * l11);
                    //     inbuffer += 2;
                    //     outbuffer += 2;
                    //     length--;
                    // }
                    // FMOD_DSP_Connection_MixStereoToStereo_SIMD(inbuffer, outbuffer, length, l00, l11);
                } else {
                    len = length >> 2;
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l00);
                        outbuffer[1] += (inbuffer[1] * l11);

                        outbuffer[2] += (inbuffer[2] * l00);
                        outbuffer[3] += (inbuffer[3] * l11);

                        outbuffer[4] += (inbuffer[4] * l00);
                        outbuffer[5] += (inbuffer[5] * l11);

                        outbuffer[6] += (inbuffer[6] * l00);
                        outbuffer[7] += (inbuffer[7] * l11);
                        inbuffer += 8;
                        outbuffer += 8;
                        len--;
                    }

                    len = length & 3;
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l00);
                        outbuffer[1] += (inbuffer[1] * l11);
                        inbuffer += 2;
                        outbuffer += 2;
                        len--;
                    }
                }
            } else {
                len = length >> 2;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[0] += (inbuffer[1] * l01);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[1] += (inbuffer[1] * l11);

                    outbuffer[2] += (inbuffer[2] * l00);
                    outbuffer[2] += (inbuffer[3] * l01);
                    outbuffer[3] += (inbuffer[2] * l10);
                    outbuffer[3] += (inbuffer[3] * l11);

                    outbuffer[4] += (inbuffer[4] * l00);
                    outbuffer[4] += (inbuffer[5] * l01);
                    outbuffer[5] += (inbuffer[4] * l10);
                    outbuffer[5] += (inbuffer[5] * l11);

                    outbuffer[6] += (inbuffer[6] * l00);
                    outbuffer[6] += (inbuffer[7] * l01);
                    outbuffer[7] += (inbuffer[6] * l10);
                    outbuffer[7] += (inbuffer[7] * l11);
                    inbuffer += 8;
                    outbuffer += 8;
                    len--;
                }

                len = length & 3;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[0] += (inbuffer[1] * l01);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[1] += (inbuffer[1] * l11);
                    inbuffer += 2;
                    outbuffer += 2;
                    len--;
                }
            }
        }
    } else if (outchannels == 6 && (inchannels == 1 || inchannels == 2 || inchannels == 6)) {
        if (inchannels == 1) {
            if (supportssimd) {
                // TODO:
                //
                // float volume0to3[4] = { l00, l10, l20, l30 };
                // float volume4to1[4] = { l40, l50, l00, l10 };
                // float volume2to5[4] = { l20, l30, l40, l50 };
                //
                // while (reinterpret_cast<uintptr_t>(outbuffer) & 0xF && length) {
                //     outbuffer[0] += (inbuffer[0] * l00);
                //     outbuffer[1] += (inbuffer[0] * l10);
                //     outbuffer[2] += (inbuffer[0] * l20);
                //     outbuffer[3] += (inbuffer[0] * l30);
                //     outbuffer[4] += (inbuffer[0] * l40);
                //     outbuffer[5] += (inbuffer[0] * l50);
                //
                //     inbuffer++;
                //     outbuffer += 6;
                //     length--;
                // }
                //
                // FMOD_DSP_Connection_MixMonoTo5_1_SIMD(inbuffer, outbuffer, length, volume0to3, volume4to1, volume2to5);
            } else {
                uint32_t len;

                len = length >> 2;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[2] += (inbuffer[0] * l20);
                    outbuffer[3] += (inbuffer[0] * l30);

                    outbuffer[4] += (inbuffer[0] * l40);
                    outbuffer[5] += (inbuffer[0] * l50);
                    outbuffer[6] += (inbuffer[1] * l00);
                    outbuffer[7] += (inbuffer[1] * l10);

                    outbuffer[8] += (inbuffer[1] * l20);
                    outbuffer[9] += (inbuffer[1] * l30);
                    outbuffer[10] += (inbuffer[1] * l40);
                    outbuffer[11] += (inbuffer[1] * l50);

                    outbuffer[12] += (inbuffer[2] * l00);
                    outbuffer[13] += (inbuffer[2] * l10);
                    outbuffer[14] += (inbuffer[2] * l20);
                    outbuffer[15] += (inbuffer[2] * l30);

                    outbuffer[16] += (inbuffer[2] * l40);
                    outbuffer[17] += (inbuffer[2] * l50);
                    outbuffer[18] += (inbuffer[3] * l00);
                    outbuffer[19] += (inbuffer[3] * l10);

                    outbuffer[20] += (inbuffer[3] * l20);
                    outbuffer[21] += (inbuffer[3] * l30);
                    outbuffer[22] += (inbuffer[3] * l40);
                    outbuffer[23] += (inbuffer[3] * l50);

                    outbuffer += 24;
                    inbuffer += 4;

                    len--;
                }

                len = length & 3;
                if (len) {
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l00);
                        outbuffer[1] += (inbuffer[0] * l10);
                        outbuffer[2] += (inbuffer[0] * l20);
                        outbuffer[3] += (inbuffer[0] * l30);
                        outbuffer[4] += (inbuffer[0] * l40);
                        outbuffer[5] += (inbuffer[0] * l50);

                        inbuffer++;
                        outbuffer += 6;
                        len--;
                    }
                }
            }
        } else if (inchannels == 2) {
            uint32_t len;

            if (l01 < DSP_LEVEL_SMALLVAL && l10 < DSP_LEVEL_SMALLVAL &&
                l20 < DSP_LEVEL_SMALLVAL && l21 < DSP_LEVEL_SMALLVAL &&
                l30 < DSP_LEVEL_SMALLVAL && l31 < DSP_LEVEL_SMALLVAL &&
                l40 < DSP_LEVEL_SMALLVAL && l41 < DSP_LEVEL_SMALLVAL &&
                l50 < DSP_LEVEL_SMALLVAL && l51 < DSP_LEVEL_SMALLVAL) {
                len = length >> 2;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[1] += (inbuffer[1] * l11);

                    outbuffer[6] += (inbuffer[2] * l00);
                    outbuffer[7] += (inbuffer[3] * l11);

                    outbuffer[12] += (inbuffer[4] * l00);
                    outbuffer[13] += (inbuffer[5] * l11);

                    outbuffer[18] += (inbuffer[6] * l00);
                    outbuffer[19] += (inbuffer[7] * l11);

                    outbuffer += 24;
                    inbuffer += 8;
                    len--;
                }

                len = length & 3;
                if (len) {
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l00);
                        outbuffer[1] += (inbuffer[1] * l11);

                        inbuffer += 2;
                        outbuffer += 6;
                        len--;
                    }
                }
            } else {
                len = length >> 1;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[0] += (inbuffer[1] * l01);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[1] += (inbuffer[1] * l11);
                    outbuffer[2] += (inbuffer[0] * l20);
                    outbuffer[2] += (inbuffer[1] * l21);
                    outbuffer[3] += (inbuffer[0] * l30);
                    outbuffer[3] += (inbuffer[1] * l31);
                    outbuffer[4] += (inbuffer[0] * l40);
                    outbuffer[4] += (inbuffer[1] * l41);
                    outbuffer[5] += (inbuffer[0] * l50);
                    outbuffer[5] += (inbuffer[1] * l51);

                    outbuffer[6] += (inbuffer[2] * l00);
                    outbuffer[6] += (inbuffer[3] * l01);
                    outbuffer[7] += (inbuffer[2] * l10);
                    outbuffer[7] += (inbuffer[3] * l11);
                    outbuffer[8] += (inbuffer[2] * l20);
                    outbuffer[8] += (inbuffer[3] * l21);
                    outbuffer[9] += (inbuffer[2] * l30);
                    outbuffer[9] += (inbuffer[3] * l31);
                    outbuffer[10] += (inbuffer[2] * l40);
                    outbuffer[10] += (inbuffer[3] * l41);
                    outbuffer[11] += (inbuffer[2] * l50);
                    outbuffer[11] += (inbuffer[3] * l51);

                    inbuffer += 4;
                    outbuffer += 12;
                    len--;
                }

                len = length & 1;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[0] += (inbuffer[1] * l01);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[1] += (inbuffer[1] * l11);
                    outbuffer[2] += (inbuffer[0] * l20);
                    outbuffer[2] += (inbuffer[1] * l21);
                    outbuffer[3] += (inbuffer[0] * l30);
                    outbuffer[3] += (inbuffer[1] * l31);
                    outbuffer[4] += (inbuffer[0] * l40);
                    outbuffer[4] += (inbuffer[1] * l41);
                    outbuffer[5] += (inbuffer[0] * l50);
                    outbuffer[5] += (inbuffer[1] * l51);

                    inbuffer += 2;
                    outbuffer += 6;
                    len--;
                }
            }
        } else if (inchannels == 6) {
            if (this->checkUnity(outchannels, inchannels) == FMOD_OK) {
                float l[6];

                l[0] = l00;
                l[1] = l11;
                l[2] = l22;
                l[3] = l33;
                l[4] = l44;
                l[5] = l55;

                if (supportssimd) {
                    // TODO:
                    //
                    // float volume0to3[4] = { l[0], l[1], l[2], l[3] };
                    // float volume4to1[4] = { l[4], l[5], l[0], l[1] };
                    // float volume2to5[4] = { l[2], l[3], l[4], l[5] };
                    //
                    // while (reinterpret_cast<uintptr_t>(outbuffer) & 0xF && length) {
                    //     outbuffer[0] += (inbuffer[0] * l[0]);
                    //     outbuffer[1] += (inbuffer[1] * l[1]);
                    //     outbuffer[2] += (inbuffer[2] * l[2]);
                    //     outbuffer[3] += (inbuffer[3] * l[3]);
                    //     outbuffer[4] += (inbuffer[4] * l[4]);
                    //     outbuffer[5] += (inbuffer[5] * l[5]);
                    //
                    //     inbuffer += 6;
                    //     outbuffer += 6;
                    //     length--;
                    // }
                    //
                    // FMOD_DSP_Connection_Mix5_1To5_1_SIMD(inbuffer, outbuffer, length, volume0to3, volume4to1, volume2to5);
                } else {
                    uint32_t len;

                    len = length >> 2;
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l[0]);
                        outbuffer[1] += (inbuffer[1] * l[1]);
                        outbuffer[2] += (inbuffer[2] * l[2]);
                        outbuffer[3] += (inbuffer[3] * l[3]);
                        outbuffer[4] += (inbuffer[4] * l[4]);
                        outbuffer[5] += (inbuffer[5] * l[5]);
                        outbuffer[6] += (inbuffer[6] * l[0]);
                        outbuffer[7] += (inbuffer[7] * l[1]);
                        outbuffer[8] += (inbuffer[8] * l[2]);
                        outbuffer[9] += (inbuffer[9] * l[3]);
                        outbuffer[10] += (inbuffer[10] * l[4]);
                        outbuffer[11] += (inbuffer[11] * l[5]);

                        outbuffer[12] += (inbuffer[12] * l[0]);
                        outbuffer[13] += (inbuffer[13] * l[1]);
                        outbuffer[14] += (inbuffer[14] * l[2]);
                        outbuffer[15] += (inbuffer[15] * l[3]);
                        outbuffer[16] += (inbuffer[16] * l[4]);
                        outbuffer[17] += (inbuffer[17] * l[5]);
                        outbuffer[18] += (inbuffer[18] * l[0]);
                        outbuffer[19] += (inbuffer[19] * l[1]);
                        outbuffer[20] += (inbuffer[20] * l[2]);
                        outbuffer[21] += (inbuffer[21] * l[3]);
                        outbuffer[22] += (inbuffer[22] * l[4]);
                        outbuffer[23] += (inbuffer[23] * l[5]);

                        outbuffer += 24;
                        inbuffer += 24;
                        len--;
                    }

                    len = length & 3;
                    if (len) {
                        while (len) {
                            outbuffer[0] += (inbuffer[0] * l[0]);
                            outbuffer[1] += (inbuffer[1] * l[1]);
                            outbuffer[2] += (inbuffer[2] * l[2]);
                            outbuffer[3] += (inbuffer[3] * l[3]);
                            outbuffer[4] += (inbuffer[4] * l[4]);
                            outbuffer[5] += (inbuffer[5] * l[5]);

                            inbuffer += 6;
                            outbuffer += 6;
                            len--;
                        }
                    }
                }
            } else {
                for (int32_t i = 0; i < length; i++) {
                    float srcval;

                    srcval = (inbuffer[0] * mLevelCurrent[0][0]);
                    srcval += (inbuffer[1] * mLevelCurrent[0][1]);
                    srcval += (inbuffer[2] * mLevelCurrent[0][2]);
                    srcval += (inbuffer[3] * mLevelCurrent[0][3]);
                    srcval += (inbuffer[4] * mLevelCurrent[0][4]);
                    srcval += (inbuffer[5] * mLevelCurrent[0][5]);
                    outbuffer[0] += srcval;

                    srcval = (inbuffer[0] * mLevelCurrent[1][0]);
                    srcval += (inbuffer[1] * mLevelCurrent[1][1]);
                    srcval += (inbuffer[2] * mLevelCurrent[1][2]);
                    srcval += (inbuffer[3] * mLevelCurrent[1][3]);
                    srcval += (inbuffer[4] * mLevelCurrent[1][4]);
                    srcval += (inbuffer[5] * mLevelCurrent[1][5]);
                    outbuffer[1] += srcval;

                    srcval = (inbuffer[0] * mLevelCurrent[2][0]);
                    srcval += (inbuffer[1] * mLevelCurrent[2][1]);
                    srcval += (inbuffer[2] * mLevelCurrent[2][2]);
                    srcval += (inbuffer[3] * mLevelCurrent[2][3]);
                    srcval += (inbuffer[4] * mLevelCurrent[2][4]);
                    srcval += (inbuffer[5] * mLevelCurrent[2][5]);
                    outbuffer[2] += srcval;

                    srcval = (inbuffer[0] * mLevelCurrent[3][0]);
                    srcval += (inbuffer[1] * mLevelCurrent[3][1]);
                    srcval += (inbuffer[2] * mLevelCurrent[3][2]);
                    srcval += (inbuffer[3] * mLevelCurrent[3][3]);
                    srcval += (inbuffer[4] * mLevelCurrent[3][4]);
                    srcval += (inbuffer[5] * mLevelCurrent[3][5]);
                    outbuffer[3] += srcval;

                    srcval = (inbuffer[0] * mLevelCurrent[4][0]);
                    srcval += (inbuffer[1] * mLevelCurrent[4][1]);
                    srcval += (inbuffer[2] * mLevelCurrent[4][2]);
                    srcval += (inbuffer[3] * mLevelCurrent[4][3]);
                    srcval += (inbuffer[4] * mLevelCurrent[4][4]);
                    srcval += (inbuffer[5] * mLevelCurrent[4][5]);
                    outbuffer[4] += srcval;

                    srcval = (inbuffer[0] * mLevelCurrent[5][0]);
                    srcval += (inbuffer[1] * mLevelCurrent[5][1]);
                    srcval += (inbuffer[2] * mLevelCurrent[5][2]);
                    srcval += (inbuffer[3] * mLevelCurrent[5][3]);
                    srcval += (inbuffer[4] * mLevelCurrent[5][4]);
                    srcval += (inbuffer[5] * mLevelCurrent[5][5]);
                    outbuffer[5] += srcval;

                    outbuffer += 6;
                    inbuffer += 6;
                }
            }
        }
    }

    else if (outchannels == 8 && (inchannels == 1 || inchannels == 2 || inchannels == 8)) {
        if (inchannels == 1) {
            if (supportssimd) {
                // TODO:
                //
                // float volume0to3[4] = { l00, l10, l20, l30 };
                // float volume4to7[4] = { l40, l50, l60, l70 };
                //
                // FMOD_DSP_Connection_MixMonoTo7_1_SIMD(inbuffer, outbuffer, length, volume0to3, volume4to7);
            } else {
                uint32_t len;

                len = length >> 1;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[2] += (inbuffer[0] * l20);
                    outbuffer[3] += (inbuffer[0] * l30);
                    outbuffer[4] += (inbuffer[0] * l40);
                    outbuffer[5] += (inbuffer[0] * l50);
                    outbuffer[6] += (inbuffer[0] * l60);
                    outbuffer[7] += (inbuffer[0] * l70);

                    outbuffer[8] += (inbuffer[1] * l00);
                    outbuffer[9] += (inbuffer[1] * l10);
                    outbuffer[10] += (inbuffer[1] * l20);
                    outbuffer[11] += (inbuffer[1] * l30);
                    outbuffer[12] += (inbuffer[1] * l40);
                    outbuffer[13] += (inbuffer[1] * l50);
                    outbuffer[14] += (inbuffer[1] * l60);
                    outbuffer[15] += (inbuffer[1] * l70);

                    outbuffer += 16;
                    inbuffer += 2;

                    len--;
                }

                len = length & 1;
                if (len) {
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l00);
                        outbuffer[1] += (inbuffer[0] * l10);
                        outbuffer[2] += (inbuffer[0] * l20);
                        outbuffer[3] += (inbuffer[0] * l30);
                        outbuffer[4] += (inbuffer[0] * l40);
                        outbuffer[5] += (inbuffer[0] * l50);
                        outbuffer[6] += (inbuffer[0] * l60);
                        outbuffer[7] += (inbuffer[0] * l70);

                        inbuffer++;
                        outbuffer += 8;
                        len--;
                    }
                }
            }
        } else if (inchannels == 2) {
            uint32_t len;

            if (l01 < DSP_LEVEL_SMALLVAL && l10 < DSP_LEVEL_SMALLVAL &&
                l20 < DSP_LEVEL_SMALLVAL && l21 < DSP_LEVEL_SMALLVAL &&
                l30 < DSP_LEVEL_SMALLVAL && l31 < DSP_LEVEL_SMALLVAL &&
                l40 < DSP_LEVEL_SMALLVAL && l41 < DSP_LEVEL_SMALLVAL &&
                l50 < DSP_LEVEL_SMALLVAL && l51 < DSP_LEVEL_SMALLVAL &&
                l60 < DSP_LEVEL_SMALLVAL && l61 < DSP_LEVEL_SMALLVAL &&
                l70 < DSP_LEVEL_SMALLVAL && l71 < DSP_LEVEL_SMALLVAL) {
                len = length >> 2;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[1] += (inbuffer[1] * l11);

                    outbuffer[8] += (inbuffer[2] * l00);
                    outbuffer[9] += (inbuffer[3] * l11);

                    outbuffer[16] += (inbuffer[4] * l00);
                    outbuffer[17] += (inbuffer[5] * l11);

                    outbuffer[24] += (inbuffer[6] * l00);
                    outbuffer[25] += (inbuffer[7] * l11);

                    outbuffer += 32;
                    inbuffer += 8;
                    len--;
                }

                len = length & 3;
                if (len) {
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l00);
                        outbuffer[1] += (inbuffer[1] * l11);

                        inbuffer += 2;
                        outbuffer += 8;
                        len--;
                    }
                }
            } else {
                len = length >> 1;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[0] += (inbuffer[1] * l01);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[1] += (inbuffer[1] * l11);
                    outbuffer[2] += (inbuffer[0] * l20);
                    outbuffer[2] += (inbuffer[1] * l21);
                    outbuffer[3] += (inbuffer[0] * l30);
                    outbuffer[3] += (inbuffer[1] * l31);
                    outbuffer[4] += (inbuffer[0] * l40);
                    outbuffer[4] += (inbuffer[1] * l41);
                    outbuffer[5] += (inbuffer[0] * l50);
                    outbuffer[5] += (inbuffer[1] * l51);
                    outbuffer[6] += (inbuffer[0] * l60);
                    outbuffer[6] += (inbuffer[1] * l61);
                    outbuffer[7] += (inbuffer[0] * l70);
                    outbuffer[7] += (inbuffer[1] * l71);

                    outbuffer[8] += (inbuffer[2] * l00);
                    outbuffer[8] += (inbuffer[3] * l01);
                    outbuffer[9] += (inbuffer[2] * l10);
                    outbuffer[9] += (inbuffer[3] * l11);
                    outbuffer[10] += (inbuffer[2] * l20);
                    outbuffer[10] += (inbuffer[3] * l21);
                    outbuffer[11] += (inbuffer[2] * l30);
                    outbuffer[11] += (inbuffer[3] * l31);
                    outbuffer[12] += (inbuffer[2] * l40);
                    outbuffer[12] += (inbuffer[3] * l41);
                    outbuffer[13] += (inbuffer[2] * l50);
                    outbuffer[13] += (inbuffer[3] * l51);
                    outbuffer[14] += (inbuffer[2] * l60);
                    outbuffer[14] += (inbuffer[3] * l61);
                    outbuffer[15] += (inbuffer[2] * l70);
                    outbuffer[15] += (inbuffer[3] * l71);

                    inbuffer += 4;
                    outbuffer += 16;
                    len--;
                }

                len = length & 1;
                while (len) {
                    outbuffer[0] += (inbuffer[0] * l00);
                    outbuffer[0] += (inbuffer[1] * l01);
                    outbuffer[1] += (inbuffer[0] * l10);
                    outbuffer[1] += (inbuffer[1] * l11);
                    outbuffer[2] += (inbuffer[0] * l20);
                    outbuffer[2] += (inbuffer[1] * l21);
                    outbuffer[3] += (inbuffer[0] * l30);
                    outbuffer[3] += (inbuffer[1] * l31);
                    outbuffer[4] += (inbuffer[0] * l40);
                    outbuffer[4] += (inbuffer[1] * l41);
                    outbuffer[5] += (inbuffer[0] * l50);
                    outbuffer[5] += (inbuffer[1] * l51);
                    outbuffer[6] += (inbuffer[0] * l60);
                    outbuffer[6] += (inbuffer[1] * l61);
                    outbuffer[7] += (inbuffer[0] * l70);
                    outbuffer[7] += (inbuffer[1] * l71);

                    inbuffer += 2;
                    outbuffer += 8;
                    len--;
                }
            }
        } else if (inchannels == 8) {
            if (this->checkUnity(outchannels, inchannels) == FMOD_OK) {
                float l[8];

                l[0] = l00;
                l[1] = l11;
                l[2] = l22;
                l[3] = l33;
                l[4] = l44;
                l[5] = l55;
                l[6] = l66;
                l[7] = l77;

                if (supportssimd) {
                    // TODO:
                    //
                    // float volume0to3[4] = { l[0], l[1], l[2], l[3] };
                    // float volume4to7[4] = { l[4], l[5], l[6], l[7] };
                    //
                    // FMOD_DSP_Connection_Mix7_1To7_1_SIMD(inbuffer, outbuffer, length, volume0to3, volume4to7);
                } else {
                    uint32_t len;

                    len = length >> 1;
                    while (len) {
                        outbuffer[0] += (inbuffer[0] * l[0]);
                        outbuffer[1] += (inbuffer[1] * l[1]);
                        outbuffer[2] += (inbuffer[2] * l[2]);
                        outbuffer[3] += (inbuffer[3] * l[3]);
                        outbuffer[4] += (inbuffer[4] * l[4]);
                        outbuffer[5] += (inbuffer[5] * l[5]);
                        outbuffer[6] += (inbuffer[6] * l[6]);
                        outbuffer[7] += (inbuffer[7] * l[7]);

                        outbuffer[8] += (inbuffer[8] * l[0]);
                        outbuffer[9] += (inbuffer[9] * l[1]);
                        outbuffer[10] += (inbuffer[10] * l[2]);
                        outbuffer[11] += (inbuffer[11] * l[3]);
                        outbuffer[12] += (inbuffer[12] * l[4]);
                        outbuffer[13] += (inbuffer[13] * l[5]);
                        outbuffer[14] += (inbuffer[14] * l[6]);
                        outbuffer[15] += (inbuffer[15] * l[7]);

                        outbuffer += 16;
                        inbuffer += 16;
                        len--;
                    }

                    len = length & 1;
                    if (len) {
                        while (len) {
                            outbuffer[0] += (inbuffer[0] * l[0]);
                            outbuffer[1] += (inbuffer[1] * l[1]);
                            outbuffer[2] += (inbuffer[2] * l[2]);
                            outbuffer[3] += (inbuffer[3] * l[3]);
                            outbuffer[4] += (inbuffer[4] * l[4]);
                            outbuffer[5] += (inbuffer[5] * l[5]);
                            outbuffer[6] += (inbuffer[6] * l[6]);
                            outbuffer[7] += (inbuffer[7] * l[7]);

                            inbuffer += 8;
                            outbuffer += 8;
                            len--;
                        }
                    }
                }
            } else {
                for (int32_t i = 0; i < length; i++) {
                    for (int32_t j = 0; j < outchannels; j++) {
                        float srcval = 0;
                        float destval = *outbuffer;

                        for (int32_t k = 0; k < inchannels; k++) {
                            srcval += (inbuffer[k] * this->mLevelCurrent[j][k]);
                        }

                        destval += srcval;
                        *outbuffer++ = destval;
                    }
                    inbuffer += inchannels;
                }
            }
        }
    } else {
        for (int32_t i = 0; i < length; i++) {
            for (int32_t j = 0; j < outchannels; j++) {
                float srcval = 0;
                float destval = *outbuffer;

                for (int32_t k = 0; k < inchannels; k++) {
                    srcval += (inbuffer[k] * this->mLevelCurrent[j][k]);
                }

                destval += srcval;
                *outbuffer++ = destval;
            }
            inbuffer += inchannels;
        }
    }

    return FMOD_OK;
}

} // namespace FMOD
