#ifndef XMODEX_DSP_DSP_FFT_HPP
#define XMODEX_DSP_DSP_FFT_HPP

#include "fmod.hpp"

namespace FMOD {

constexpr int32_t DSPFFT_COSTABBITS = 13;
constexpr int32_t DSPFFT_COSTABSIZE = (1 << DSPFFT_COSTABBITS);
constexpr int32_t DSPFFT_TABLERANGE = (DSPFFT_COSTABSIZE * 4);
constexpr int32_t DSPFFT_TABLEMASK = (DSPFFT_TABLERANGE - 1);

class DSPFFT {
    private:
        FMOD_COMPLEX mFFTBuffer[16384];
        float mCosTab[DSPFFT_COSTABSIZE];

    private:
        inline float cosine(float x) {
            int32_t y;

            x *= DSPFFT_TABLERANGE;
            y = static_cast<int32_t>(x);
            if (y < 0) {
                y = -y;
            }

            y &= DSPFFT_TABLEMASK;
            switch (y >> DSPFFT_COSTABBITS) {
            case 0:
                return this->mCosTab[y];
            case 1:
                return -this->mCosTab[(DSPFFT_COSTABSIZE - 1) - (y - (DSPFFT_COSTABSIZE * 1))];
            case 2:
                return -this->mCosTab[(y - (DSPFFT_COSTABSIZE * 2))];
            case 3:
                return this->mCosTab[(DSPFFT_COSTABSIZE - 1) - (y - (DSPFFT_COSTABSIZE * 3))];
            }

            return 0.0f;
        }

        inline float sine(float x) {
            return this->cosine(x - 0.25f);
        }

        inline uint32_t reverse(uint32_t val, int32_t bits) {
            uint32_t retn = 0;

            while (bits--) {
                retn <<= 1;
                retn |= (val & 1);
                val >>= 1;
            }

            return retn;
        }

        FMOD_RESULT process(int32_t bits);

    public:
        DSPFFT();

        FMOD_RESULT getSpectrum(
            float* pcmbuffer,
            uint32_t pcmposition,
            uint32_t pcmlength,
            float* spectrum,
            int32_t length,
            int32_t channel,
            int32_t numchannels,
            FMOD_DSP_FFT_WINDOW windowtype);
};

} // namespace FMOD

#endif
