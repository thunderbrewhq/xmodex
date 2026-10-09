#include "xmodex/codec/IMAAdpcm.hpp"

namespace FMOD {

static const int32_t IMAAdpcm_IndexTab[16] = {
    -1, -1, -1, -1,
    2, 4, 6, 8,
    -1, -1, -1, -1,
    2, 4, 6, 8
};

static const int16_t IMAAdpcm_StepTab[89] = {
    7, 8, 9, 10, 11, 12, 13,
    14, 16, 17, 19, 21, 23, 25,
    28, 31, 34, 37, 41, 45, 50,
    55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190,
    209, 230, 253, 279, 307, 337, 371,
    408, 449, 494, 544, 598, 658, 724,
    796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749,
    3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767
};

inline int32_t IMAAdpcm_NextStepIndex(int32_t nEncodedSample, int32_t nStepIndex) {
    nStepIndex += IMAAdpcm_IndexTab[nEncodedSample];

    if (nStepIndex < 0) {
        nStepIndex = 0;
    } else if (nStepIndex >= static_cast<int32_t>(sizeof(IMAAdpcm_StepTab) / sizeof(IMAAdpcm_StepTab[0]))) {
        nStepIndex = (sizeof(IMAAdpcm_StepTab) / sizeof(IMAAdpcm_StepTab[0])) - 1;
    }

    return nStepIndex;
}

inline int8_t ValidStepIndex(int32_t nStepIndex) {
    return (nStepIndex >= 0) && (nStepIndex < static_cast<int32_t>(sizeof(IMAAdpcm_StepTab) / sizeof(IMAAdpcm_StepTab[0])));
}

inline int32_t IMAAdpcm_DecodeSample(int32_t nEncodedSample, int32_t nPredictedSample, int32_t nStepSize) {
    int32_t lDifference;
    int32_t lNewSample;

    lDifference = nStepSize >> 3;

    switch (nEncodedSample) {
    case 0: {
        break;
    }
    case 1: {
        lDifference += nStepSize >> 2;
        break;
    }
    case 2: {
        lDifference += nStepSize >> 1;
        break;
    }
    case 3: {
        lDifference += nStepSize >> 1;
        lDifference += nStepSize >> 2;
        break;
    }
    case 4: {
        lDifference += nStepSize;
        break;
    }
    case 5: {
        lDifference += nStepSize;
        lDifference += nStepSize >> 2;
        break;
    }
    case 6: {
        lDifference += nStepSize;
        lDifference += nStepSize >> 1;
        break;
    }
    case 7: {
        lDifference += nStepSize;
        lDifference += nStepSize >> 1;
        lDifference += nStepSize >> 2;
        break;
    }
    case 8: {
        lDifference = -lDifference;
        break;
    }
    case 9: {
        lDifference += nStepSize >> 2;
        lDifference = -lDifference;
        break;
    }
    case 10: {
        lDifference += nStepSize >> 1;
        lDifference = -lDifference;
        break;
    }
    case 11: {
        lDifference += nStepSize >> 1;
        lDifference += nStepSize >> 2;
        lDifference = -lDifference;
        break;
    }
    case 12: {
        lDifference += nStepSize;
        lDifference = -lDifference;
        break;
    }
    case 13: {
        lDifference += nStepSize;
        lDifference += nStepSize >> 2;
        lDifference = -lDifference;
        break;
    }
    case 14: {
        lDifference += nStepSize;
        lDifference += nStepSize >> 1;
        lDifference = -lDifference;
        break;
    }
    case 15: {
        lDifference += nStepSize;
        lDifference += nStepSize >> 1;
        lDifference += nStepSize >> 2;
        lDifference = -lDifference;
        break;
    }
    default: {
        break;
    }
    }

    lNewSample = nPredictedSample + lDifference;

    if (static_cast<int32_t>(static_cast<int16_t>(lNewSample)) != lNewSample) {
        if (lNewSample < -32768) {
            lNewSample = -32768;
        } else {
            lNewSample = 32767;
        }
    }

    return static_cast<int32_t>(lNewSample);
}

FMOD_RESULT IMAAdpcm_DecodeM16(
    uint8_t* pbSrc,
    int16_t* pbDst,
    uint32_t cBlocks,
    uint32_t nBlockAlignment,
    uint32_t cSamplesPerBlock,
    int32_t channels) {

    FMOD_RESULT result = FMOD_OK;
    int32_t nEncSample;
    int32_t nPredSample;
    int32_t nStepIndex;
    int32_t nStepSize;
    uint32_t cSamples;
    uint32_t dwHeader;
    uint8_t bSample;
    uint8_t* pbBlock;

    while (cBlocks--) {
        pbBlock = pbSrc;
        cSamples = cSamplesPerBlock - 1;

        dwHeader = *reinterpret_cast<uint32_t*>(pbBlock);

#if defined(XMODEX_CPU_BIGENDIAN)
        dwHeader = FMOD_SWAPENDIAN_DWORD(dwHeader);
#endif
        pbBlock += sizeof(uint32_t);

        nPredSample = static_cast<int32_t>(static_cast<int16_t>(static_cast<uint16_t>(dwHeader)));
        nStepIndex = static_cast<int32_t>(static_cast<uint8_t>(static_cast<uint16_t>(static_cast<uint32_t>(dwHeader) >> 16) & 0xFFFF));

        if (!ValidStepIndex(nStepIndex)) {
            result = FMOD_ERR_FILE_BAD;
            break;
        }

        *pbDst = static_cast<int16_t>(nPredSample);
        pbDst += channels;

        while (cSamples) {
            bSample = *pbBlock++;

            nEncSample = bSample & static_cast<uint8_t>(0xF);
            nStepSize = IMAAdpcm_StepTab[nStepIndex];
            nPredSample = IMAAdpcm_DecodeSample(nEncSample, nPredSample, nStepSize);
            nStepIndex = IMAAdpcm_NextStepIndex(nEncSample, nStepIndex);

            *pbDst = static_cast<int16_t>(nPredSample);
            pbDst += channels;

            cSamples--;

            if (cSamples) {
                nEncSample = bSample >> 4;
                nStepSize = IMAAdpcm_StepTab[nStepIndex];
                nPredSample = IMAAdpcm_DecodeSample(nEncSample, nPredSample, nStepSize);
                nStepIndex = IMAAdpcm_NextStepIndex(nEncSample, nStepIndex);

                *pbDst = static_cast<int16_t>(nPredSample);
                pbDst += channels;

                cSamples--;
            }
        }

        pbSrc += nBlockAlignment;
    }

    return result;
}

FMOD_RESULT IMAAdpcm_DecodeS16(
    uint8_t* pbSrc,
    int16_t* pbDst,
    uint32_t cBlocks,
    uint32_t nBlockAlignment,
    uint32_t cSamplesPerBlock) {
    FMOD_RESULT result = FMOD_OK;
    uint8_t* pbBlock;
    uint32_t cSamples;
    uint32_t cSubSamples;
    int32_t nStepSize;
    uint32_t dwHeader;
    uint32_t dwLeft;
    uint32_t dwRight;
    int32_t nEncSampleL;
    int32_t nPredSampleL;
    int32_t nStepIndexL;
    int32_t nEncSampleR;
    int32_t nPredSampleR;
    int32_t nStepIndexR;

    while (cBlocks--) {
        pbBlock = pbSrc;
        cSamples = cSamplesPerBlock - 1;

        dwHeader = *reinterpret_cast<uint32_t*>(pbBlock);
#if defined(XMODEX_CPU_BIGENDIAN)
        dwHeader = FMOD_SWAPENDIAN_DWORD(dwHeader);
#endif
        pbBlock += sizeof(uint32_t);

        nPredSampleL = static_cast<int32_t>(static_cast<int16_t>(static_cast<uint16_t>(dwHeader)));
        nStepIndexL = static_cast<int32_t>(static_cast<uint8_t>(static_cast<uint16_t>((static_cast<uint32_t>(dwHeader) >> 16) & 0xFFFF)));

        if (!ValidStepIndex(nStepIndexL)) {
            result = FMOD_ERR_FILE_BAD;
            break;
        }

        dwHeader = *reinterpret_cast<uint32_t*>(pbBlock);
#if defined(XMODEX_CPU_BIGENDIAN)
        dwHeader = FMOD_SWAPENDIAN_DWORD(dwHeader);
#endif
        pbBlock += sizeof(uint32_t);

        nPredSampleR = static_cast<int32_t>(static_cast<int16_t>(static_cast<uint16_t>(dwHeader)));
        nStepIndexR = static_cast<int32_t>(static_cast<uint8_t>(static_cast<uint16_t>((static_cast<uint32_t>(dwHeader) >> 16) & 0xFFFF)));

        if (!ValidStepIndex(nStepIndexR)) {
            result = FMOD_ERR_FILE_BAD;
            break;
        }

        *pbDst++ = nPredSampleL;
        *pbDst++ = nPredSampleR;

        while (cSamples) {
            dwLeft = *reinterpret_cast<uint32_t*>(pbBlock);
#if defined(XMODEX_CPU_BIGENDIAN)
            dwLeft = FMOD_SWAPENDIAN_DWORD(dwLeft);
#endif
            pbBlock += sizeof(uint32_t);
            dwRight = *reinterpret_cast<uint32_t*>(pbBlock);
#if defined(XMODEX_CPU_BIGENDIAN)
            dwRight = FMOD_SWAPENDIAN_DWORD(dwRight);
#endif
            pbBlock += sizeof(uint32_t);

            cSubSamples = 8;
            if (cSamples < cSubSamples) {
                cSubSamples = cSamples;
            }

            for (uint32_t i = 0; i < cSubSamples; i++) {
                nEncSampleL = dwLeft & 0xF;
                nStepSize = IMAAdpcm_StepTab[nStepIndexL];
                nPredSampleL = IMAAdpcm_DecodeSample(nEncSampleL, nPredSampleL, nStepSize);
                nStepIndexL = IMAAdpcm_NextStepIndex(nEncSampleL, nStepIndexL);

                nEncSampleR = dwRight & 0xF;
                nStepSize = IMAAdpcm_StepTab[nStepIndexR];
                nPredSampleR = IMAAdpcm_DecodeSample(nEncSampleR, nPredSampleR, nStepSize);
                nStepIndexR = IMAAdpcm_NextStepIndex(nEncSampleR, nStepIndexR);

                *pbDst++ = nPredSampleL;
                *pbDst++ = nPredSampleR;

                dwLeft >>= 4;
                dwRight >>= 4;
            }

            cSamples -= cSubSamples;
        }

        pbSrc += nBlockAlignment;
    }

    return result;
}

} // namespace FMOD
