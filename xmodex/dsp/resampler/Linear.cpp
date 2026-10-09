#include "xmodex/dsp/resampler/Linear.hpp"
// #include "xmodex/os/SIMD.hpp"

#define OO4GIG (1.0f / 4294967296.0f)

void FMOD_Resampler_Linear(
    float* out,
    int32_t outlength,
    void* src,
    FMOD_SOUND_FORMAT srcformat,
    FMOD_UINT64P* position,
    FMOD_SINT64P* speed,
    int32_t channels) {
    float scale = 1.0f;

    bool supportssimd;

    // TODO:
    //
    // supportssimd = FMOD_OS_SupportsSIMD();

    supportssimd = false;

    switch (srcformat) {
    case FMOD_SOUND_FORMAT_PCM8: {
        auto inptr = reinterpret_cast<int8_t*>(src);

        scale /= static_cast<float>(1 << 7);

        if (channels == 1) {
            float f;
            float a;
            float b;
            float r1;
            float r2;
            float r3;
            float r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r1 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r2 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r3 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r4 = (a * (1.0f - f)) + (b * f);
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
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r1 = (a * (1.0f - f)) + (b * f);

                *out++ = r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else if (channels == 2) {
            float f;
            float l_a;
            float l_b;
            float l_r1;
            float l_r2;
            float l_r3;
            float l_r4;
            float r_a;
            float r_b;
            float r_r1;
            float r_r2;
            float r_r3;
            float r_r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                l_r1 = (l_a * (1.0f - f)) + (l_b * f);
                r_r1 = (r_a * (1.0f - f)) + (r_b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                l_r2 = (l_a * (1.0f - f)) + (l_b * f);
                r_r2 = (r_a * (1.0f - f)) + (r_b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                l_r3 = (l_a * (1.0f - f)) + (l_b * f);
                r_r3 = (r_a * (1.0f - f)) + (r_b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                l_r4 = (l_a * (1.0f - f)) + (l_b * f);
                r_r4 = (r_a * (1.0f - f)) + (r_b * f);
                position->mValue += speed->mValue;

                out[0] = l_r1;
                out[1] = r_r1;
                out[2] = l_r2;
                out[3] = r_r2;
                out[4] = l_r3;
                out[5] = r_r3;
                out[6] = l_r4;
                out[7] = r_r4;

                len--;
                out += 8;
            }

            len = outlength & 3;
            while (len) {
                f = position->mLo * OO4GIG;
                l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                l_r1 = (l_a * (1.0f - f)) + (l_b * f);
                r_r1 = (r_a * (1.0f - f)) + (r_b * f);

                *out++ = l_r1;
                *out++ = r_r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float p0 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    float p1 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    float r;

                    r = (p0 * (1.0f - f)) + (p1 * f);
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

        if (channels == 1) {
            if (supportssimd) {
                // TODO:
                //
                // FMOD_Resampler_Linear_PCM16_Mono(out, outlength, src, position, speed);
            } else {
                float f;
                float a;
                float b;
                float r1;
                float r2;
                float r3;
                float r4;
                int32_t len;

                scale /= static_cast<float>(1 << 15);
                len = outlength >> 2;
                while (len) {
                    f = static_cast<float>(position->mLo) * OO4GIG;
                    a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                    b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                    r1 = (a * (1.0f - f)) + (b * f);
                    position->mValue += speed->mValue;

                    f = static_cast<float>(position->mLo) * OO4GIG;
                    a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                    b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                    r2 = (a * (1.0f - f)) + (b * f);
                    position->mValue += speed->mValue;

                    f = static_cast<float>(position->mLo) * OO4GIG;
                    a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                    b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                    r3 = (a * (1.0f - f)) + (b * f);
                    position->mValue += speed->mValue;

                    f = static_cast<float>(position->mLo) * OO4GIG;
                    a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                    b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                    r4 = (a * (1.0f - f)) + (b * f);
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
                    a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                    b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                    r1 = (a * (1.0f - f)) + (b * f);

                    *out++ = r1;

                    position->mValue += speed->mValue;
                    len--;
                }
            }
        } else if (channels == 2) {
            if (supportssimd) {
                // TODO:
                //
                // FMOD_Resampler_Linear_PCM16_Stereo(out, outlength, src, position, speed);
            } else {
                float f;
                float l_a;
                float l_b;
                float l_r1;
                float l_r2;
                float l_r3;
                float l_r4;
                float r_a;
                float r_b;
                float r_r1;
                float r_r2;
                float r_r3;
                float r_r4;
                int32_t len;

                scale /= static_cast<float>(1 << 15);
                len = outlength >> 2;

                while (len) {
                    f = static_cast<float>(position->mLo) * OO4GIG;
                    l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                    l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                    r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                    r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                    l_r1 = (l_a * (1.0f - f)) + (l_b * f);
                    r_r1 = (r_a * (1.0f - f)) + (r_b * f);
                    position->mValue += speed->mValue;

                    f = static_cast<float>(position->mLo) * OO4GIG;
                    l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                    l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                    r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                    r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                    l_r2 = (l_a * (1.0f - f)) + (l_b * f);
                    r_r2 = (r_a * (1.0f - f)) + (r_b * f);
                    position->mValue += speed->mValue;

                    f = static_cast<float>(position->mLo) * OO4GIG;
                    l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                    l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                    r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                    r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                    l_r3 = (l_a * (1.0f - f)) + (l_b * f);
                    r_r3 = (r_a * (1.0f - f)) + (r_b * f);
                    position->mValue += speed->mValue;

                    f = static_cast<float>(position->mLo) * OO4GIG;
                    l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                    l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                    r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                    r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                    l_r4 = (l_a * (1.0f - f)) + (l_b * f);
                    r_r4 = (r_a * (1.0f - f)) + (r_b * f);
                    position->mValue += speed->mValue;

                    out[0] = l_r1;
                    out[1] = r_r1;
                    out[2] = l_r2;
                    out[3] = r_r2;
                    out[4] = l_r3;
                    out[5] = r_r3;
                    out[6] = l_r4;
                    out[7] = r_r4;
                    len--;
                    out += 8;
                }

                len = outlength & 3;
                while (len) {
                    f = position->mLo * OO4GIG;
                    l_a = static_cast<float>(inptr[position->mHi * 2 + 0]) * scale;
                    l_b = static_cast<float>(inptr[position->mHi * 2 + 2]) * scale;
                    r_a = static_cast<float>(inptr[position->mHi * 2 + 1]) * scale;
                    r_b = static_cast<float>(inptr[position->mHi * 2 + 3]) * scale;
                    l_r1 = (l_a * (1.0f - f)) + (l_b * f);
                    r_r1 = (r_a * (1.0f - f)) + (r_b * f);

                    out[0] = l_r1;
                    out[1] = r_r1;

                    position->mValue += speed->mValue;
                    len--;
                    out += 2;
                }
            }
        } else {
            scale /= static_cast<float>(1 << 15);

            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float p0 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    float p1 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    float r;

                    r = (p0 * (1.0f - f)) + (p1 * f);
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
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    FMOD_INT24* s0 = &inptr[position->mHi + 0];
                    FMOD_INT24* s1 = &inptr[position->mHi + 1];
                    float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;
                    float p1 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s1->val[0]) << 8) | (static_cast<uint32_t>(s1->val[1]) << 16) | (static_cast<uint32_t>(s1->val[2]) << 24)) >> 8) * scale;

                    r = (p0 * (1.0f - f)) + (p1 * f);

                    *out++ = r;
                }
                position->mValue += speed->mValue;
                outlength--;
            }
        } else {
            while (outlength) {
                float r;
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    FMOD_INT24* s0 = &inptr[((position->mHi + 0) * channels) + i];
                    FMOD_INT24* s1 = &inptr[((position->mHi + 1) * channels) + i];
                    float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;
                    float p1 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s1->val[0]) << 8) | (static_cast<uint32_t>(s1->val[1]) << 16) | (static_cast<uint32_t>(s1->val[2]) << 24)) >> 8) * scale;

                    r = (p0 * (1.0f - f)) + (p1 * f);

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

        scale /= static_cast<float>(1 << 31);

        if (channels == 1) {
            float f;
            float a;
            float b;
            float r1;
            float r2;
            float r3;
            float r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r1 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r2 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r3 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r4 = (a * (1.0f - f)) + (b * f);
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
                a = static_cast<float>(inptr[position->mHi + 0]) * scale;
                b = static_cast<float>(inptr[position->mHi + 1]) * scale;
                r1 = (a * (1.0f - f)) + (b * f);

                *out++ = r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float p0 = static_cast<float>(inptr[((position->mHi + 0) * channels) + i]) * scale;
                    float p1 = static_cast<float>(inptr[((position->mHi + 1) * channels) + i]) * scale;
                    float r;

                    r = (p0 * (1.0f - f)) + (p1 * f);
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
            float a;
            float b;
            float r1;
            float r2;
            float r3;
            float r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                f = static_cast<float>(position->mLo) * OO4GIG;
                a = inptr[position->mHi + 0];
                b = inptr[position->mHi + 1];
                r1 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = inptr[position->mHi + 0];
                b = inptr[position->mHi + 1];
                r2 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = inptr[position->mHi + 0];
                b = inptr[position->mHi + 1];
                r3 = (a * (1.0f - f)) + (b * f);
                position->mValue += speed->mValue;

                f = static_cast<float>(position->mLo) * OO4GIG;
                a = inptr[position->mHi + 0];
                b = inptr[position->mHi + 1];
                r4 = (a * (1.0f - f)) + (b * f);
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
                a = inptr[position->mHi + 0];
                b = inptr[position->mHi + 1];
                r1 = (a * (1.0f - f)) + (b * f);

                *out++ = r1;

                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                float f = position->mLo * OO4GIG;

                for (int32_t i = 0; i < channels; i++) {
                    float p0 = inptr[((position->mHi + 0) * channels) + i];
                    float p1 = inptr[((position->mHi + 1) * channels) + i];
                    float r;

                    r = (p0 * (1.0f - f)) + (p1 * f);
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
