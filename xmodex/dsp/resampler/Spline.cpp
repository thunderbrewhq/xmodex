#include "xmodex/dsp/resampler/Spline.hpp"

#define OO4GIG (1.0f / 4294967296.0f)

void FMOD_Resampler_Spline(
    float* out,
    int32_t outlength,
    void* src,
    FMOD_SOUND_FORMAT srcformat,
    FMOD_UINT64P* position,
    FMOD_SINT64P* speed,
    int32_t channels) {
    float scale = 1.0f;

    switch (srcformat) {
    case FMOD_SOUND_FORMAT_PCM8: {
        auto inptr = reinterpret_cast<int8_t*>(src);

        scale /= static_cast<float>(1 << 7);

        if (channels == 1) {
            while (outlength) {
                float f = position->mLo * OO4GIG;
                float r;
                float p0 = static_cast<float>(inptr[position->mHi - 2]) * scale;
                float p1 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                float p2 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                float p3 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                float p4 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                float p5 = static_cast<float>(inptr[position->mHi + 3]) * scale;

                r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                *out++ = r;

                position->mValue += speed->mValue;
                outlength--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float r;
                    float p0 = static_cast<float>(inptr[((position->mHi - 2) * channels) + i]) * scale;
                    float p1 = static_cast<float>(inptr[((position->mHi - 1) * channels) + i]) * scale;
                    float p2 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    float p3 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    float p4 = static_cast<float>(inptr[((position->mHi + 2) * channels) + i]) * scale;
                    float p5 = static_cast<float>(inptr[((position->mHi + 3) * channels) + i]) * scale;

                    r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                    *out++ = r;
                }
                position->mValue += speed->mValue;
                outlength--;
            }
        }
        break;
    }

    case FMOD_SOUND_FORMAT_PCM16: {
        auto inptr = reinterpret_cast<int16_t*>(src);

        scale /= static_cast<float>(1 << 15);

        if (channels == 1) {
            while (outlength) {
                float f = position->mLo * OO4GIG;
                float r;
                float p0 = static_cast<float>(inptr[position->mHi - 2]) * scale;
                float p1 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                float p2 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                float p3 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                float p4 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                float p5 = static_cast<float>(inptr[position->mHi + 3]) * scale;

                r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                *out++ = r;

                position->mValue += speed->mValue;
                outlength--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float r;
                    float p0 = static_cast<float>(inptr[((position->mHi - 2) * channels) + i]) * scale;
                    float p1 = static_cast<float>(inptr[((position->mHi - 1) * channels) + i]) * scale;
                    float p2 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    float p3 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    float p4 = static_cast<float>(inptr[((position->mHi + 2) * channels) + i]) * scale;
                    float p5 = static_cast<float>(inptr[((position->mHi + 3) * channels) + i]) * scale;

                    r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                    *out++ = r;
                }
                position->mValue += speed->mValue;
                outlength--;
            }
        }
        break;
    }

    case FMOD_SOUND_FORMAT_PCM24: {
        auto inptr = reinterpret_cast<FMOD_INT24*>(src);

        scale /= static_cast<float>(1 << 23);

        if (channels == 1) {
            while (outlength) {
                float f = position->mLo * OO4GIG;
                float r;
                FMOD_INT24* s0 = &inptr[position->mHi - 2];
                FMOD_INT24* s1 = &inptr[position->mHi - 1];
                FMOD_INT24* s2 = &inptr[position->mHi + 0];
                FMOD_INT24* s3 = &inptr[position->mHi + 1];
                FMOD_INT24* s4 = &inptr[position->mHi + 2];
                FMOD_INT24* s5 = &inptr[position->mHi + 3];
                float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;
                float p1 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s1->val[0]) << 8) | (static_cast<uint32_t>(s1->val[1]) << 16) | (static_cast<uint32_t>(s1->val[2]) << 24)) >> 8) * scale;
                float p2 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s2->val[0]) << 8) | (static_cast<uint32_t>(s2->val[1]) << 16) | (static_cast<uint32_t>(s2->val[2]) << 24)) >> 8) * scale;
                float p3 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s3->val[0]) << 8) | (static_cast<uint32_t>(s3->val[1]) << 16) | (static_cast<uint32_t>(s3->val[2]) << 24)) >> 8) * scale;
                float p4 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s4->val[0]) << 8) | (static_cast<uint32_t>(s4->val[1]) << 16) | (static_cast<uint32_t>(s4->val[2]) << 24)) >> 8) * scale;
                float p5 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s5->val[0]) << 8) | (static_cast<uint32_t>(s5->val[1]) << 16) | (static_cast<uint32_t>(s5->val[2]) << 24)) >> 8) * scale;

                r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                *out++ = r;

                position->mValue += speed->mValue;
                outlength--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float r;
                    FMOD_INT24* s0 = &inptr[((position->mHi - 2) * channels) + i];
                    FMOD_INT24* s1 = &inptr[((position->mHi - 1) * channels) + i];
                    FMOD_INT24* s2 = &inptr[((position->mHi + 0) * channels) + i];
                    FMOD_INT24* s3 = &inptr[((position->mHi + 1) * channels) + i];
                    FMOD_INT24* s4 = &inptr[((position->mHi + 2) * channels) + i];
                    FMOD_INT24* s5 = &inptr[((position->mHi + 3) * channels) + i];
                    float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;
                    float p1 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s1->val[0]) << 8) | (static_cast<uint32_t>(s1->val[1]) << 16) | (static_cast<uint32_t>(s1->val[2]) << 24)) >> 8) * scale;
                    float p2 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s2->val[0]) << 8) | (static_cast<uint32_t>(s2->val[1]) << 16) | (static_cast<uint32_t>(s2->val[2]) << 24)) >> 8) * scale;
                    float p3 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s3->val[0]) << 8) | (static_cast<uint32_t>(s3->val[1]) << 16) | (static_cast<uint32_t>(s3->val[2]) << 24)) >> 8) * scale;
                    float p4 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s4->val[0]) << 8) | (static_cast<uint32_t>(s4->val[1]) << 16) | (static_cast<uint32_t>(s4->val[2]) << 24)) >> 8) * scale;
                    float p5 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s5->val[0]) << 8) | (static_cast<uint32_t>(s5->val[1]) << 16) | (static_cast<uint32_t>(s5->val[2]) << 24)) >> 8) * scale;

                    r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                    *out++ = r;
                }
                position->mValue += speed->mValue;
                outlength--;
            }
        }
        break;
    }

    case FMOD_SOUND_FORMAT_PCM32: {
        auto inptr = reinterpret_cast<int32_t*>(src);

        scale /= static_cast<float>(1U << 31);

        if (channels == 1) {
            while (outlength) {
                float f = position->mLo * OO4GIG;
                float r;
                float p0 = static_cast<float>(inptr[position->mHi - 2]) * scale;
                float p1 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                float p2 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                float p3 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                float p4 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                float p5 = static_cast<float>(inptr[position->mHi + 3]) * scale;

                r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                *out++ = r;

                position->mValue += speed->mValue;
                outlength--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float r;
                    float p0 = static_cast<float>(inptr[((position->mHi - 2) * channels) + i]) * scale;
                    float p1 = static_cast<float>(inptr[((position->mHi - 1) * channels) + i]) * scale;
                    float p2 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    float p3 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    float p4 = static_cast<float>(inptr[((position->mHi + 2) * channels) + i]) * scale;
                    float p5 = static_cast<float>(inptr[((position->mHi + 3) * channels) + i]) * scale;

                    r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                    *out++ = r;
                }
                position->mValue += speed->mValue;
                outlength--;
            }
        }
        break;
    }

    case FMOD_SOUND_FORMAT_PCMFLOAT: {
        auto inptr = reinterpret_cast<float*>(src);

        if (channels == 1) {
            while (outlength) {
                float f = position->mLo * OO4GIG;
                float r;
                float p0 = inptr[position->mHi - 2];
                float p1 = inptr[position->mHi - 1];
                float p2 = inptr[position->mHi + 0];
                float p3 = inptr[position->mHi + 1];
                float p4 = inptr[position->mHi + 2];
                float p5 = inptr[position->mHi + 3];

                r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                *out++ = r;

                position->mValue += speed->mValue;
                outlength--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float r;
                    float p0 = inptr[((position->mHi - 2) * channels) + i];
                    float p1 = inptr[((position->mHi - 1) * channels) + i];
                    float p2 = inptr[((position->mHi + 0) * channels) + i];
                    float p3 = inptr[((position->mHi + 1) * channels) + i];
                    float p4 = inptr[((position->mHi + 2) * channels) + i];
                    float p5 = inptr[((position->mHi + 3) * channels) + i];

                    r = p2 + 0.04166666666f * f * ((p3 - p1) * 16.0f + (p0 - p4) * 2.0f + f * ((p3 + p1) * 16.0f - p0 - p2 * 30.0f - p4 + f * (p3 * 66.0f - p2 * 70.0f - p4 * 33.0f + p1 * 39.0f + p5 * 7.0f - p0 * 9.0f + f * (p2 * 126.0f - p3 * 124.0f + p4 * 61.0f - p1 * 64.0f - p5 * 12.0f + p0 * 13.0f + f * ((p3 - p2) * 50.0f + (p1 - p4) * 25.0f + (p5 - p0) * 5.0f)))));

                    *out++ = r;
                }
                position->mValue += speed->mValue;
                outlength--;
            }
        }
        break;
    }
    default: {
        break;
    }
    }
}
