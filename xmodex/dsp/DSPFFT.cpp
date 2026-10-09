#include "xmodex/dsp/DSPFFT.hpp"
#include "xmodex/Math.hpp"

namespace FMOD {

DSPFFT::DSPFFT() {
    for (int32_t i = 0; i < DSPFFT_COSTABSIZE; i++) {
        this->mCosTab[i] = FMOD_COS(static_cast<float>(i) * 0.0001917476f);
    }
}

FMOD_RESULT DSPFFT::process(int32_t bits) {
    uint32_t i1;
    int32_t i2;
    int32_t fftlen = 1 << bits;
    float oneoverN = 1.0f / fftlen;

    i1 = fftlen / 2;
    i2 = 1;

    for (int32_t i = 0; i < bits; i++) {
        int32_t i3 = 0;
        int32_t i4 = i1;

        for (int32_t j = 0; j < i2; j++) {
            int32_t y = this->reverse(i3 / static_cast<int32_t>(i1), bits);
            float z1 = this->cosine(static_cast<float>(y) * oneoverN);
            float z2 = -this->sine(static_cast<float>(y) * oneoverN);

            for (int32_t k = i3; k < i4; k++) {
                float a1 = this->mFFTBuffer[k].re;
                float a2 = this->mFFTBuffer[k].im;
                float b1 = (z1 * this->mFFTBuffer[k + i1].re) - (z2 * this->mFFTBuffer[k + i1].im);
                float b2 = (z2 * this->mFFTBuffer[k + i1].re) + (z1 * this->mFFTBuffer[k + i1].im);

                this->mFFTBuffer[k].re = a1 + b1;
                this->mFFTBuffer[k].im = a2 + b2;
                this->mFFTBuffer[k + i1].re = a1 - b1;
                this->mFFTBuffer[k + i1].im = a2 - b2;
            }

            i3 += (i1 << 1);
            i4 += (i1 << 1);
        }

        i1 >>= 1;
        i2 <<= 1;
    }

    return FMOD_OK;
}

FMOD_RESULT DSPFFT::getSpectrum(
    float* pcmbuffer,
    uint32_t pcmposition,
    uint32_t pcmlength,
    float* spectrum,
    int32_t length,
    int32_t channel,
    int32_t numchannels,
    FMOD_DSP_FFT_WINDOW windowtype) {

    int32_t bits;
    int32_t bitslength;
    int32_t nyquist;

    bitslength = length;
    bits = 0;
    while (bitslength > 1) {
        bitslength >>= 1;
        bits++;
    }

    switch (windowtype) {
    case FMOD_DSP_FFT_WINDOW_TRIANGLE: {
        for (int32_t i = 0; i < length; i++) {
            float window;
            float percent = static_cast<float>(i) / static_cast<float>(length);

            window = (percent * 2.0f) - 1.0f;
            window = FMOD_ABS(window);
            window = 1.0f - window;

            this->mFFTBuffer[i].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
            this->mFFTBuffer[i].re /= static_cast<float>(length);
            this->mFFTBuffer[i].im = 0.00000001f;

            pcmposition++;
            if (pcmposition >= pcmlength) {
                pcmposition = 0;
            }
        }
        break;
    }
    case FMOD_DSP_FFT_WINDOW_HAMMING: {
        for (int32_t i = 0; i < length; i++) {
            float window;
            float percent = static_cast<float>(i) / static_cast<float>(length);

            window = 0.54f - (0.46f * this->cosine(percent));

            this->mFFTBuffer[i].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
            this->mFFTBuffer[i].re /= static_cast<float>(length);
            this->mFFTBuffer[i].im = 0.00000001f;

            pcmposition++;
            if (pcmposition >= pcmlength) {
                pcmposition = 0;
            }
        }
        break;
    }
    case FMOD_DSP_FFT_WINDOW_HANNING: {
        for (int32_t i = 0; i < length; i++) {
            float window;
            float percent = static_cast<float>(i) / static_cast<float>(length);

            window = 0.5f * (1.0f - this->cosine(percent));

            this->mFFTBuffer[i].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
            this->mFFTBuffer[i].re /= static_cast<float>(length);
            this->mFFTBuffer[i].im = 0.00000001f;

            pcmposition++;
            if (pcmposition >= pcmlength) {
                pcmposition = 0;
            }
        }
        break;
    }
    case FMOD_DSP_FFT_WINDOW_BLACKMAN: {
        for (int32_t i = 0; i < length; i++) {
            float window;
            float percent = static_cast<float>(i) / static_cast<float>(length);

            window = 0.42f - (0.5f * this->cosine(percent)) + (0.08f * this->cosine(2.0f * percent));

            this->mFFTBuffer[i].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
            this->mFFTBuffer[i].re /= static_cast<float>(length);
            this->mFFTBuffer[i].im = 0.00000001f;

            pcmposition++;
            if (pcmposition >= pcmlength) {
                pcmposition = 0;
            }
        }
        break;
    }
    case FMOD_DSP_FFT_WINDOW_BLACKMANHARRIS: {
        float a0 = 0.35875f;
        float a1 = 0.48829f;
        float a2 = 0.14128f;
        float a3 = 0.01168f;

        for (int32_t i = 0; i < length; i++) {
            float window;
            float percent = static_cast<float>(i) / static_cast<float>(length);

            window = a0 - (a1 * this->cosine(1.0f * percent)) +
                     (a2 * this->cosine(2.0f * percent)) -
                     (a3 * this->cosine(3.0f * percent));

            this->mFFTBuffer[i].re = pcmbuffer[(pcmposition * numchannels) + channel] * window;
            this->mFFTBuffer[i].re /= static_cast<float>(length);
            this->mFFTBuffer[i].im = 0.00000001f;

            pcmposition++;
            if (pcmposition >= pcmlength) {
                pcmposition = 0;
            }
        }
        break;
    }
    case FMOD_DSP_FFT_WINDOW_RECT:
    default: {
        for (int32_t i = 0; i < length; i++) {
            this->mFFTBuffer[i].re = pcmbuffer[(pcmposition * numchannels) + channel];
            this->mFFTBuffer[i].re /= static_cast<float>(length);
            this->mFFTBuffer[i].im = 0.00000001f;

            pcmposition++;
            if (pcmposition >= pcmlength) {
                pcmposition = 0;
            }
        }
        break;
    }
    }

    this->process(bits);

    nyquist = length / 2;
    for (int32_t i = 0; i < nyquist - 1; i++) {
        float magnitude;
        int32_t n = i;

        n = this->reverse(n, bits);

        magnitude = FMOD_SQRT((this->mFFTBuffer[n].re * this->mFFTBuffer[n].re) + (this->mFFTBuffer[n].im * this->mFFTBuffer[n].im));

        magnitude *= 2.5f;

        if (magnitude > 1.0f) {
            magnitude = 1.0f;
        }

        spectrum[i] = magnitude;
    }

    return FMOD_OK;
}

} // namespace FMOD
