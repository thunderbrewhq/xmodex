#include "xmodex/dsp/resampler/Cubic.hpp"

#define OO4GIG (1.0f / 4294967296.0f)

void FMOD_Resampler_Cubic(
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
            float f;
            float p0;
            float p1;
            float p2;
            float p3;
            float r1;
            float r2;
            float r3;
            float r4;
            float a;
            float b;
            float c;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r2 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r3 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r4 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                out[0] = r1;
                out[1] = r2;
                out[2] = r3;
                out[3] = r4;
                len--;
                out += 4;
            }

            len = outlength & 3;
            while (len) {
                f = position->mLo * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;

                *out++ = r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                float p0;
                float p1;
                float p2;
                float p3;
                float r;
                float a;
                float b;
                float c;
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    p0 = static_cast<float>(inptr[((position->mHi - 1) * channels) + i]) * scale;
                    p1 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    p2 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    p3 = static_cast<float>(inptr[((position->mHi + 2) * channels) + i]) * scale;
                    a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r = (((a * f) + b) * f + c) * f + p1;
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
            float f;
            float p0;
            float p1;
            float p2;
            float p3;
            float r1;
            float r2;
            float r3;
            float r4;
            float a;
            float b;
            float c;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r2 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r3 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r4 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                out[0] = r1;
                out[1] = r2;
                out[2] = r3;
                out[3] = r4;
                len--;
                out += 4;
            }

            len = outlength & 3;
            while (len) {
                f = position->mLo * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;

                *out++ = r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                float p0;
                float p1;
                float p2;
                float p3;
                float r;
                float a;
                float b;
                float c;
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    p0 = static_cast<float>(inptr[((position->mHi - 1) * channels) + i]) * scale;
                    p1 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    p2 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    p3 = static_cast<float>(inptr[((position->mHi + 2) * channels) + i]) * scale;
                    a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r = (((a * f) + b) * f + c) * f + p1;
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
                float r;
                float a;
                float b;
                float c;
                float f = position->mLo * OO4GIG;
                FMOD_INT24* s0 = &inptr[position->mHi - 1];
                FMOD_INT24* s1 = &inptr[position->mHi + 0];
                FMOD_INT24* s2 = &inptr[position->mHi + 1];
                FMOD_INT24* s3 = &inptr[position->mHi + 2];
                float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;
                float p1 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s1->val[0]) << 8) | (static_cast<uint32_t>(s1->val[1]) << 16) | (static_cast<uint32_t>(s1->val[2]) << 24)) >> 8) * scale;
                float p2 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s2->val[0]) << 8) | (static_cast<uint32_t>(s2->val[1]) << 16) | (static_cast<uint32_t>(s2->val[2]) << 24)) >> 8) * scale;
                float p3 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s3->val[0]) << 8) | (static_cast<uint32_t>(s3->val[1]) << 16) | (static_cast<uint32_t>(s3->val[2]) << 24)) >> 8) * scale;

                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r = (((a * f) + b) * f + c) * f + p1;
                *out++ = r;

                position->mValue += speed->mValue;
                outlength--;
            }
        } else {
            while (outlength) {
                float r;
                float a;
                float b;
                float c;
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    FMOD_INT24* s0 = &inptr[((position->mHi - 1) * channels) + i];
                    FMOD_INT24* s1 = &inptr[((position->mHi + 0) * channels) + i];
                    FMOD_INT24* s2 = &inptr[((position->mHi + 1) * channels) + i];
                    FMOD_INT24* s3 = &inptr[((position->mHi + 2) * channels) + i];
                    float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;
                    float p1 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s1->val[0]) << 8) | (static_cast<uint32_t>(s1->val[1]) << 16) | (static_cast<uint32_t>(s1->val[2]) << 24)) >> 8) * scale;
                    float p2 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s2->val[0]) << 8) | (static_cast<uint32_t>(s2->val[1]) << 16) | (static_cast<uint32_t>(s2->val[2]) << 24)) >> 8) * scale;
                    float p3 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s3->val[0]) << 8) | (static_cast<uint32_t>(s3->val[1]) << 16) | (static_cast<uint32_t>(s3->val[2]) << 24)) >> 8) * scale;

                    a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r = (((a * f) + b) * f + c) * f + p1;
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
            float f;
            float p0;
            float p1;
            float p2;
            float p3;
            float r1;
            float r2;
            float r3;
            float r4;
            float a;
            float b;
            float c;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r2 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r3 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r4 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                out[0] = r1;
                out[1] = r2;
                out[2] = r3;
                out[3] = r4;
                len--;
                out += 4;
            }

            len = outlength & 3;
            while (len) {
                f = position->mLo * OO4GIG;
                p0 = static_cast<float>(inptr[position->mHi - 1]) * scale;
                p1 = static_cast<float>(inptr[position->mHi + 0]) * scale;
                p2 = static_cast<float>(inptr[position->mHi + 1]) * scale;
                p3 = static_cast<float>(inptr[position->mHi + 2]) * scale;
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;

                *out++ = r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                float p0;
                float p1;
                float p2;
                float p3;
                float r;
                float a;
                float b;
                float c;
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    p0 = static_cast<float>(inptr[((position->mHi - 1) * channels) + i]) * scale;
                    p1 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    p2 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    p3 = static_cast<float>(inptr[((position->mHi + 2) * channels) + i]) * scale;
                    a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r = (((a * f) + b) * f + c) * f + p1;
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
            float f;
            float p0;
            float p1;
            float p2;
            float p3;
            float r1;
            float r2;
            float r3;
            float r4;
            float a;
            float b;
            float c;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = inptr[position->mHi - 1];
                p1 = inptr[position->mHi + 0];
                p2 = inptr[position->mHi + 1];
                p3 = inptr[position->mHi + 2];
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = inptr[position->mHi - 1];
                p1 = inptr[position->mHi + 0];
                p2 = inptr[position->mHi + 1];
                p3 = inptr[position->mHi + 2];
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r2 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = inptr[position->mHi - 1];
                p1 = inptr[position->mHi + 0];
                p2 = inptr[position->mHi + 1];
                p3 = inptr[position->mHi + 2];
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r3 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                p0 = inptr[position->mHi - 1];
                p1 = inptr[position->mHi + 0];
                p2 = inptr[position->mHi + 1];
                p3 = inptr[position->mHi + 2];
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r4 = (((a * f) + b) * f + c) * f + p1;
                position->mValue += speed->mValue;

                out[0] = r1;
                out[1] = r2;
                out[2] = r3;
                out[3] = r4;
                len--;
                out += 4;
            }

            len = outlength & 3;
            while (len) {
                f = position->mLo * OO4GIG;
                p0 = inptr[position->mHi - 1];
                p1 = inptr[position->mHi + 0];
                p2 = inptr[position->mHi + 1];
                p3 = inptr[position->mHi + 2];
                a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                c = (p2 - p0) / 2.0f;
                r1 = (((a * f) + b) * f + c) * f + p1;

                *out++ = r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                float p0;
                float p1;
                float p2;
                float p3;
                float r;
                float a;
                float b;
                float c;
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    p0 = inptr[((position->mHi - 1) * channels) + i];
                    p1 = inptr[((position->mHi + 0) * channels) + i];
                    p2 = inptr[((position->mHi + 1) * channels) + i];
                    p3 = inptr[((position->mHi + 2) * channels) + i];
                    a = (3.0f * (p1 - p2) - p0 + p3) / 2.0f;
                    b = 2.0f * p2 + p0 - (5.0f * p1 + p3) / 2.0f;
                    c = (p2 - p0) / 2.0f;
                    r = (((a * f) + b) * f + c) * f + p1;
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
