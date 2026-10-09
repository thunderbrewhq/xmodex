#include "xmodex/dsp/DSPPitchShiftSMB.hpp"
#include "fmod.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/String.hpp"

#define kINVPI 0.3183098861837F
#define kINVPI2 0.1591549430918F

namespace FMOD {

float gFFTworksp[MAX_FRAME_LENGTH * 2];

float gFFTtable[MAX_FRAME_LENGTH / 2];

int32_t gFFTbitrev[2 + (1 << MAX_FRAME_LENGTH_HALF_BITS)];

float gSynFreq[MAX_FRAME_LENGTH];

float gSynMagn[MAX_FRAME_LENGTH];

float gAnaFreq[MAX_FRAME_LENGTH];

float gAnaMagn[MAX_FRAME_LENGTH];

void DSPPitchShiftSMB::smbInit() {
    FMOD_memset(this->mInFIFO, 0, MAX_FRAME_LENGTH * sizeof(float));
    FMOD_memset(this->mOutFIFO, 0, MAX_FRAME_LENGTH * sizeof(float));
    FMOD_memset(this->mLastPhase, 0, (4 + (MAX_FRAME_LENGTH / 2)) * sizeof(float));
    FMOD_memset(this->mSumPhase, 0, (4 + (MAX_FRAME_LENGTH / 2)) * sizeof(float));
    FMOD_memset(this->mOutputAccum, 0, (2 * MAX_FRAME_LENGTH) * sizeof(float));

    FMOD_memset(gFFTworksp, 0, 2 * MAX_FRAME_LENGTH * sizeof(float));
    FMOD_memset(gAnaFreq, 0, MAX_FRAME_LENGTH * sizeof(float));
    FMOD_memset(gAnaMagn, 0, MAX_FRAME_LENGTH * sizeof(float));
    FMOD_memset(gSynFreq, 0, MAX_FRAME_LENGTH * sizeof(float));
    FMOD_memset(gSynMagn, 0, MAX_FRAME_LENGTH * sizeof(float));

    this->mRover = false;
}

void DSPPitchShiftSMB::smbFft(float* fftBuffer, int32_t sign) {
    float wr;
    float wi;
    float arg;
    float* p1;
    float* p2;
    float temp;
    float tr;
    float ti;
    float ur;
    float ui;
    float* p1r;
    float* p1i;
    float* p2r;
    float* p2i;
    int32_t i;
    int32_t bitm;
    int32_t j;
    int32_t le;
    int32_t le2;
    int32_t k;

    for (i = 2; i < 2 * this->mFFTFrameSize - 2; i += 2) {
        for (bitm = 2, j = 0; bitm < 2 * this->mFFTFrameSize; bitm <<= 1) {
            if (i & bitm) {
                j++;
            }
            j <<= 1;
        }
        if (i < j) {
            p1 = fftBuffer + i;
            p2 = fftBuffer + j;

            temp = p1[0];
            p1[0] = p2[0];
            p2[0] = temp;

            temp = p1[1];
            p1[1] = p2[1];
            p2[1] = temp;

            p1++;
            p2++;
        }
    }

    for (k = 0, le = 2; k < this->mFFTFrameBits; k++) {
        le <<= 1;
        le2 = le >> 1;
        ur = 1.0f;
        ui = 0.0f;

        arg = 0.5f / (le2 >> 1);

        wr = this->cosine(arg);
        wi = sign * this->sine(arg);

        for (j = 0; j < le2; j += 2) {
            p1r = fftBuffer + j;
            p1i = p1r + 1;
            p2r = p1r + le2;
            p2i = p2r + 1;

            for (i = j; i < 2 * this->mFFTFrameSize; i += le) {
                tr = p2r[0] * ur - p2i[0] * ui;
                ti = p2r[0] * ui + p2i[0] * ur;

                p2r[0] = p1r[0] - tr;
                p2i[0] = p1i[0] - ti;

                p1r[0] += tr;
                p1i[0] += ti;

                p1r += le;
                p1i += le;
                p2r += le;
                p2i += le;
            }
            tr = ur * wr - ui * wi;
            ui = ur * wi + ui * wr;
            ur = tr;
        }
    }
}

void DSPPitchShiftSMB::smbPitchShift(float pitchShift, int32_t numSampsToProcess, int32_t osamp, float sampleRate, float* indata, float* outdata, int32_t channel, int32_t numchannels) {
    float window;
    float freqPerBin;
    float expct;
    int32_t i;
    int32_t k;
    int32_t index;
    int32_t inFifoLatency;
    int32_t stepSize;
    int32_t fftFrameSize2;
    int32_t fadeZoneLen;
    float* inptr;
    float sum;

    inptr = indata + channel;
    sum = 0;

    for (i = 0; i < numSampsToProcess; i++) {
        sum += FMOD_FABS(inptr[0]);
        inptr += numchannels;
    }

    if (sum < .001f) {
        auto outptr = outdata + channel;

        for (i = 0; i < numSampsToProcess; i++) {
            outptr[0] = 0;
            outptr += numchannels;
        }
        return;
    }

    fadeZoneLen = this->mFFTFrameSize / 2;
    fftFrameSize2 = this->mFFTFrameSize / 2;
    stepSize = this->mFFTFrameSize / osamp;
    freqPerBin = sampleRate / static_cast<float>(this->mFFTFrameSize);
    expct = FMOD_PI2 * static_cast<float>(stepSize) / static_cast<float>(this->mFFTFrameSize);
    inFifoLatency = this->mFFTFrameSize - stepSize;

    if (!this->mRover) {
        this->mRover = inFifoLatency;
    }

    if (this->mResetPhaseFlag) {
        FMOD_memset(this->mLastPhase, 0, (4 + (MAX_FRAME_LENGTH / 2)) * sizeof(float));
        FMOD_memset(this->mSumPhase, 0, (4 + (MAX_FRAME_LENGTH / 2)) * sizeof(float));
        this->mResetPhaseFlag = false;
    }

    for (i = 0; i < numSampsToProcess; i++) {
        this->mInFIFO[this->mRover] = indata[i * numchannels + channel];
        outdata[i * numchannels + channel] = this->mOutFIFO[this->mRover - inFifoLatency];
        this->mRover++;

        if (this->mRover >= this->mFFTFrameSize) {
            this->mRover = inFifoLatency;

            for (k = 0; k < this->mFFTFrameSize; k++) {
                window = this->mWindow[k];

                gFFTworksp[2 * k] = this->mInFIFO[k] * window;
                gFFTworksp[2 * k + 1] = 0.0f;
            }

            this->smbFft(gFFTworksp, -1);

            float phase0;
            float phase1;
            float phase2;
            float phase3;
            float tmp0;
            float tmp1;
            float tmp2;
            float tmp3;
            float real0;
            float real1;
            float real2;
            float real3;
            float imag0;
            float imag1;
            float imag2;
            float imag3;
            int32_t qpd0;
            int32_t qpd1;
            int32_t qpd2;
            int32_t qpd3;
            int32_t k0;
            int32_t k1;
            int32_t k2;
            int32_t k3;

            float factor = freqPerBin * osamp / FMOD_PI2;

            for (k = 0; k <= fftFrameSize2; k += 4) {
                k0 = k;
                k1 = k + 1;
                k2 = k + 2;
                k3 = k + 3;

                real0 = gFFTworksp[2 * k0];
                imag0 = gFFTworksp[2 * k0 + 1];
                real1 = gFFTworksp[2 * k1];
                imag1 = gFFTworksp[2 * k1 + 1];
                real2 = gFFTworksp[2 * k2];
                imag2 = gFFTworksp[2 * k2 + 1];
                real3 = gFFTworksp[2 * k3];
                imag3 = gFFTworksp[2 * k3 + 1];

                phase0 = smbAtan2(imag0, real0);
                phase1 = smbAtan2(imag1, real1);
                phase2 = smbAtan2(imag2, real2);
                phase3 = smbAtan2(imag3, real3);

                FMOD_VECTOR v0 = { real0, imag0, 0 };
                gAnaMagn[k0] = 2.0f * FMOD_Vector_GetLength(&v0);
                FMOD_VECTOR v1 = { real1, imag1, 0 };
                gAnaMagn[k1] = 2.0f * FMOD_Vector_GetLength(&v1);
                FMOD_VECTOR v2 = { real2, imag2, 0 };
                gAnaMagn[k2] = 2.0f * FMOD_Vector_GetLength(&v2);
                FMOD_VECTOR v3 = { real3, imag3, 0 };
                gAnaMagn[k3] = 2.0f * FMOD_Vector_GetLength(&v3);

                tmp0 = -(static_cast<float>(k0) * expct + this->mLastPhase[k0]);
                tmp1 = -(static_cast<float>(k1) * expct + this->mLastPhase[k1]);
                tmp2 = -(static_cast<float>(k2) * expct + this->mLastPhase[k2]);
                tmp3 = -(static_cast<float>(k3) * expct + this->mLastPhase[k3]);

                tmp0 += phase0;
                this->mLastPhase[k0] = phase0;
                tmp1 += phase1;
                this->mLastPhase[k1] = phase1;
                tmp2 += phase2;
                this->mLastPhase[k2] = phase2;
                tmp3 += phase3;
                this->mLastPhase[k3] = phase3;

                qpd0 = static_cast<int32_t>(tmp0 * kINVPI);
                if (qpd0 >= 0) {
                    qpd0 += qpd0 & 1;
                } else {
                    qpd0 -= qpd0 & 1;
                }
                tmp0 -= FMOD_PI * static_cast<float>(qpd0);

                qpd1 = static_cast<int32_t>(tmp1 * kINVPI);
                if (qpd1 >= 0) {
                    qpd1 += qpd1 & 1;
                } else {
                    qpd1 -= qpd1 & 1;
                }
                tmp1 -= FMOD_PI * static_cast<float>(qpd1);

                qpd2 = static_cast<int32_t>(tmp2 * kINVPI);
                if (qpd2 >= 0) {
                    qpd2 += qpd2 & 1;
                } else {
                    qpd2 -= qpd2 & 1;
                }
                tmp2 -= FMOD_PI * static_cast<float>(qpd2);

                qpd3 = static_cast<int32_t>(tmp3 * kINVPI);
                if (qpd3 >= 0) {
                    qpd3 += qpd3 & 1;
                } else {
                    qpd3 -= qpd3 & 1;
                }
                tmp3 -= FMOD_PI * static_cast<float>(qpd3);

                gAnaFreq[k0] = static_cast<float>(k0) * freqPerBin + tmp0 * factor;
                gAnaFreq[k1] = static_cast<float>(k1) * freqPerBin + tmp1 * factor;
                gAnaFreq[k2] = static_cast<float>(k2) * freqPerBin + tmp2 * factor;
                gAnaFreq[k3] = static_cast<float>(k3) * freqPerBin + tmp3 * factor;
            }

            FMOD_memset(gSynMagn, 0, this->mFFTFrameSize * sizeof(float));
            FMOD_memset(gSynFreq, 0, this->mFFTFrameSize * sizeof(float));

            for (k = 0; k <= fftFrameSize2; k++) {
                index = static_cast<int32_t>((k * pitchShift) + 0.5f);
                if (index <= fftFrameSize2) {
                    gSynFreq[index] = gAnaFreq[k] * pitchShift;
                    gSynMagn[index] += gAnaMagn[k];
                }
            }

            float phasemul = FMOD_PI2 / osamp;

            for (k = 0; k <= fftFrameSize2; k += 4) {
                k0 = k;
                k1 = k + 1;
                k2 = k + 2;
                k3 = k + 3;

                this->mSumPhase[k0] += k0 * expct + phasemul * ((gSynFreq[k0] / freqPerBin) - k0);
                this->mSumPhase[k1] += k1 * expct + phasemul * ((gSynFreq[k1] / freqPerBin) - k1);
                this->mSumPhase[k2] += k2 * expct + phasemul * ((gSynFreq[k2] / freqPerBin) - k2);
                this->mSumPhase[k3] += k3 * expct + phasemul * ((gSynFreq[k3] / freqPerBin) - k3);

                if (this->mSumPhase[k0] > FMOD_PI) {
                    this->mSumPhase[k0] -= (static_cast<int32_t>(this->mSumPhase[k0] * kINVPI2)) * FMOD_PI2;
                } else if (this->mSumPhase[k0] < -FMOD_PI) {
                    this->mSumPhase[k0] -= (static_cast<int32_t>(this->mSumPhase[k0] * kINVPI2)) * FMOD_PI2;
                }

                if (this->mSumPhase[k1] > FMOD_PI) {
                    this->mSumPhase[k1] -= (static_cast<int32_t>(this->mSumPhase[k1] * kINVPI2)) * FMOD_PI2;
                } else if (this->mSumPhase[k1] < -FMOD_PI) {
                    this->mSumPhase[k1] -= (static_cast<int32_t>(this->mSumPhase[k1] * kINVPI2)) * FMOD_PI2;
                }

                if (this->mSumPhase[k2] > FMOD_PI) {
                    this->mSumPhase[k2] -= (static_cast<int32_t>(this->mSumPhase[k2] * kINVPI2)) * FMOD_PI2;
                } else if (this->mSumPhase[k2] < -FMOD_PI) {
                    this->mSumPhase[k2] -= (static_cast<int32_t>(this->mSumPhase[k2] * kINVPI2)) * FMOD_PI2;
                }

                if (this->mSumPhase[k3] > FMOD_PI) {
                    this->mSumPhase[k3] -= (static_cast<int32_t>(this->mSumPhase[k3] * kINVPI2)) * FMOD_PI2;
                } else if (this->mSumPhase[k3] < -FMOD_PI) {
                    this->mSumPhase[k3] -= (static_cast<int32_t>(this->mSumPhase[k3] * kINVPI2)) * FMOD_PI2;
                }

                phase0 = this->mSumPhase[k0] * kINVPI2;
                phase1 = this->mSumPhase[k1] * kINVPI2;
                phase2 = this->mSumPhase[k2] * kINVPI2;
                phase3 = this->mSumPhase[k3] * kINVPI2;

                gFFTworksp[2 * k0] = gSynMagn[k0] * this->cosine(phase0);
                gFFTworksp[2 * k0 + 1] = gSynMagn[k0] * this->sine(phase0);
                gFFTworksp[2 * k1] = gSynMagn[k1] * this->cosine(phase1);
                gFFTworksp[2 * k1 + 1] = gSynMagn[k1] * this->sine(phase1);
                gFFTworksp[2 * k2] = gSynMagn[k2] * this->cosine(phase2);
                gFFTworksp[2 * k2 + 1] = gSynMagn[k2] * this->sine(phase2);
                gFFTworksp[2 * k3] = gSynMagn[k3] * this->cosine(phase3);
                gFFTworksp[2 * k3 + 1] = gSynMagn[k3] * this->sine(phase3);
            }
            FMOD_memset(&gFFTworksp[this->mFFTFrameSize + 2], 0, sizeof(float) * (this->mFFTFrameSize - 2));

            this->smbFft(gFFTworksp, 1);

            for (k = 0; k < this->mFFTFrameSize; k++) {
                this->mOutputAccum[k] += this->mWindow[k] * gFFTworksp[2 * k];
            }

            for (k = 0; k < stepSize; k++) {
                this->mOutFIFO[k] = this->mOutputAccum[k];
            }

            memmove(this->mOutputAccum, this->mOutputAccum + stepSize, inFifoLatency * sizeof(float));
            FMOD_memset(this->mOutputAccum + inFifoLatency, 0, stepSize * sizeof(float));

            for (k = 0; k < inFifoLatency; k++) {
                this->mInFIFO[k] = this->mInFIFO[k + stepSize];
            }
        }
    }
}

void DSPPitchShiftSMB::setResetPhaseFlag() {
    this->mResetPhaseFlag = true;
}

} // namespace FMOD
