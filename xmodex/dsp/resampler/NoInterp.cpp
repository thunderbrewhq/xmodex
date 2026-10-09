#include "xmodex/dsp/resampler/NoInterp.hpp"

void FMOD_Resampler_NoInterp(float* out, int32_t outlength, void* src, FMOD_SOUND_FORMAT srcformat, FMOD_UINT64P* position, FMOD_SINT64P* speed, int32_t channels) {
    float scale = 1.0f;

    switch (srcformat) {
    case FMOD_SOUND_FORMAT_PCM8: {
        auto inptr = reinterpret_cast<int8_t*>(src);

        scale /= static_cast<float>(1 << 7);

        if (channels == 1) {
            float r1;
            float r2;
            float r3;
            float r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                r1 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r2 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r3 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r4 = static_cast<float>(inptr[position->mHi]) * scale;
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
                *out++ = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                for (int32_t i = 0; i < channels; i++) {
                    *out++ = static_cast<float>(inptr[(position->mHi * channels) + i]) * scale;
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
            float r1;
            float r2;
            float r3;
            float r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                r1 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r2 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r3 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r4 = static_cast<float>(inptr[position->mHi]) * scale;
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
                *out++ = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                for (int32_t i = 0; i < channels; i++) {
                    *out++ = static_cast<float>(inptr[(position->mHi * channels) + i]) * scale;
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
            int32_t len;

            len = outlength >> 2;
            while (len) {
                FMOD_INT24* s;
                float p0;
                float p1;
                float p2;
                float p3;

                s = &inptr[position->mHi];
                p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s->val[0]) << 8) | (static_cast<uint32_t>(s->val[1]) << 16) | (static_cast<uint32_t>(s->val[2]) << 24)) >> 8) * scale;
                position->mValue += speed->mValue;
                s = &inptr[position->mHi];
                p1 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s->val[0]) << 8) | (static_cast<uint32_t>(s->val[1]) << 16) | (static_cast<uint32_t>(s->val[2]) << 24)) >> 8) * scale;
                position->mValue += speed->mValue;
                s = &inptr[position->mHi];
                p2 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s->val[0]) << 8) | (static_cast<uint32_t>(s->val[1]) << 16) | (static_cast<uint32_t>(s->val[2]) << 24)) >> 8) * scale;
                position->mValue += speed->mValue;
                s = &inptr[position->mHi];
                p3 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s->val[0]) << 8) | (static_cast<uint32_t>(s->val[1]) << 16) | (static_cast<uint32_t>(s->val[2]) << 24)) >> 8) * scale;
                position->mValue += speed->mValue;

                out[0] = p0;
                out[1] = p1;
                out[2] = p2;
                out[3] = p3;
                out += 4;
                len--;
            }

            len = outlength & 3;
            while (len) {
                FMOD_INT24* s0 = &inptr[position->mHi];
                float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;

                *out++ = p0;
                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                for (int32_t i = 0; i < channels; i++) {
                    FMOD_INT24* s0 = &inptr[(position->mHi * channels) + i];
                    float p0 = static_cast<float>(static_cast<int32_t>((static_cast<uint32_t>(s0->val[0]) << 8) | (static_cast<uint32_t>(s0->val[1]) << 16) | (static_cast<uint32_t>(s0->val[2]) << 24)) >> 8) * scale;

                    *out++ = p0;
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
            float r1;
            float r2;
            float r3;
            float r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                r1 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r2 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r3 = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                r4 = static_cast<float>(inptr[position->mHi]) * scale;
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
                *out++ = static_cast<float>(inptr[position->mHi]) * scale;
                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                for (int32_t i = 0; i < channels; i++) {
                    *out++ = static_cast<float>(inptr[(position->mHi * channels) + i]) * scale;
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
            float r1;
            float r2;
            float r3;
            float r4;
            int32_t len;

            len = outlength >> 2;
            while (len) {
                r1 = inptr[position->mHi];
                position->mValue += speed->mValue;
                r2 = inptr[position->mHi];
                position->mValue += speed->mValue;
                r3 = inptr[position->mHi];
                position->mValue += speed->mValue;
                r4 = inptr[position->mHi];
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
                *out++ = inptr[position->mHi];
                position->mValue += speed->mValue;
                len--;
            }
        } else {
            while (outlength) {
                for (int32_t i = 0; i < channels; i++) {
                    *out++ = inptr[(position->mHi * channels) + i];
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
