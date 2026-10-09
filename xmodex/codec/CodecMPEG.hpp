#ifndef XMODEX_CODEC_CODEC_MPEG_HPP
#define XMODEX_CODEC_CODEC_MPEG_HPP

#include "xmodex/SyncPoint.hpp"
#include "xmodex/codec/Codec.hpp"
#include <cstdint>

namespace FMOD {

constexpr int32_t SBLIMIT = 32;
constexpr int32_t SSLIMIT = 18;

constexpr int32_t SCALE_BLOCK = 12;

constexpr int32_t MPG_MD_STEREO = 0;
constexpr int32_t MPG_MD_JOINT_STEREO = 1;
constexpr int32_t MPG_MD_DUAL_CHANNEL = 2;
constexpr int32_t MPG_MD_MONO = 3;

#define MAXFRAMESIZE 1792

struct al_table {
        int16_t bits;
        int16_t d;
};

struct MPEG_FRAME {
        al_table* alloc;
        int32_t stereo;
        int32_t jsbound;
        int32_t II_sblimit;
        int32_t lsf;
        int32_t mpeg25;
        int32_t header_change;
        int32_t lay;
        int32_t error_protection;
        int32_t bitrate_index;
        int32_t sampling_frequency;
        int32_t padding;
        int32_t extension;
        int32_t mode;
        int32_t mode_ext;
        int32_t copyright;
        int32_t original;
        int32_t emphasis;
        int32_t framesize;
};

struct gr_info_s {
        int32_t scfsi;
        uint32_t part2_3_length;
        uint32_t big_values;
        uint32_t scalefac_compress;
        uint32_t block_type;
        uint32_t mixed_block_flag;
        uint32_t table_select[3];
        uint32_t subblock_gain[3];
        uint32_t maxband[3];
        uint32_t maxbandl;
        uint32_t maxb;
        uint32_t region1start;
        uint32_t region2start;
        uint32_t preflag;
        uint32_t scalefac_scale;
        uint32_t count1table_select;
        float* full_gain[3];
        float* pow2gain;
};

struct III_sideinfo_ch {
        gr_info_s gr[2];
};

struct III_sideinfo {
        uint32_t main_data_begin;
        uint32_t private_bits;
        III_sideinfo_ch ch[2];
};

struct bandInfoStruct {
        int32_t longIdx[23];
        int32_t longDiff[22];
        int32_t shortIdx[14];
        int32_t shortDiff[13];
};

struct bitstream_info {
        int32_t mBitIndex;
        uint8_t* mWordPointer;
};

struct CodecMPEG_MemoryBlock {
        int32_t mFrameSize;
        int32_t mFrameSizeOld;
        MPEG_FRAME mFrame;
        uint32_t mFrameHeader;
        uint32_t mNumFrames;
        uint32_t* mFrameOffset;
        int32_t mPcmPoint;
        int32_t mLayer;
        uint8_t mBSSpace[2][2304];
        uint32_t mBSNum;
        float mSynthBuffsMem[1152];
        int32_t mSynthBo;
        bitstream_info mBSI;
        float mBlock[2][2][576];
        int32_t mBlc[2];
        uint8_t mXingToc[100];
        bool mHasXingNumFrames;
        bool mHasXingToc;
};

class CodecMPEG : public Codec {
    public:
        CodecMPEG_MemoryBlock* mMemoryBlock;
        CodecMPEG_MemoryBlock* mMemoryBlockMemory;
        SyncPoint* mSyncPoint;
        int32_t mNumSyncPoints;
        uint32_t mPCMFrameLengthBytes;
        uint8_t* mPCMBufferMemory;
        int32_t mChannels;

    public:
        FMOD_RESULT III_antialias(float xr[SBLIMIT][SSLIMIT], gr_info_s* gr_info);
        FMOD_RESULT III_dequantize_sample(float xr[SBLIMIT][SSLIMIT], int32_t* scf, gr_info_s* gr_info, int32_t sfreq, int32_t part2bits);
        FMOD_RESULT III_dequantize_sample_ms(float xr[2][SBLIMIT][SSLIMIT], int32_t* scf, gr_info_s* gr_info, int32_t sfreq, int32_t part2bits);
        FMOD_RESULT III_get_scale_factors_1(int32_t* scf, gr_info_s* gr_info, int32_t* numbits);
        FMOD_RESULT III_get_scale_factors_2(int32_t* scf, gr_info_s* gr_info, int32_t i_stereo, int32_t* numbits);
        FMOD_RESULT III_get_side_info_1(III_sideinfo* si, int32_t stereo, int32_t ms_stereo, int32_t sfreq);
        FMOD_RESULT III_get_side_info_2(III_sideinfo* si, int32_t stereo, int32_t ms_stereo, int32_t sfreq);
        FMOD_RESULT III_hybrid(float fsIn[SBLIMIT][SSLIMIT], float tsOut[SSLIMIT][SBLIMIT], int32_t ch, gr_info_s* gr_info);
        FMOD_RESULT III_i_stereo(float xr_buf[2][SBLIMIT][SSLIMIT], int32_t* scalefac, gr_info_s* gr_info, int32_t sfreq, int32_t ms_stereo, int32_t lsf);
        FMOD_RESULT II_step_one(uint32_t* bit_alloc, int32_t* scale);
        FMOD_RESULT II_step_two(uint32_t* bit_alloc, float fraction[2][4][SBLIMIT], int32_t* scale, int32_t x1);
        FMOD_RESULT closeInternal();
        FMOD_RESULT decodeFrame(uint8_t* in, void* out, uint32_t* outlen);
        FMOD_RESULT decodeHeader(void* in, int32_t* samplerate, int32_t* channels, int32_t* framesize);
        FMOD_RESULT decodeLayer2(void* pcm_sample, uint32_t* outlen);
        FMOD_RESULT decodeLayer3(void* pcm_sample, uint32_t* outlen);
        FMOD_RESULT decodeXingHeader(uint8_t* in, uint8_t* toc, uint32_t* numframes);
        FMOD_RESULT getIIStuff();
        FMOD_RESULT getPCMLength();
        FMOD_RESULT resetFrame();
        FMOD_RESULT synth(void* samples, float* bandPtr, int32_t channels, int32_t channelskip);
        FMOD_RESULT synthC(float* b0, int32_t bo1, int32_t channelskip, int16_t* samples);
        uint32_t getBits(int32_t number_of_bits);
        uint32_t getBitsFast(int32_t number_of_bits);
        inline uint32_t get1Bit() {
            uint8_t rval;

            rval = *(this->mMemoryBlock->mBSI.mWordPointer) << this->mMemoryBlock->mBSI.mBitIndex;

            this->mMemoryBlock->mBSI.mBitIndex++;
            this->mMemoryBlock->mBSI.mWordPointer += (this->mMemoryBlock->mBSI.mBitIndex >> 3);
            this->mMemoryBlock->mBSI.mBitIndex &= 7;

            return static_cast<uint32_t>(rval >> 7);
        }
        inline int32_t getByte() {
            return *this->mMemoryBlock->mBSI.mWordPointer++;
        }
        inline void backbits(int32_t number_of_bits) {
            this->mMemoryBlock->mBSI.mBitIndex -= number_of_bits;
            this->mMemoryBlock->mBSI.mWordPointer += (this->mMemoryBlock->mBSI.mBitIndex >> 3);
            this->mMemoryBlock->mBSI.mBitIndex &= 0x7;
        }

        static FMOD_RESULT initAll();
        static FMOD_RESULT initLayer2();
        static FMOD_RESULT initLayer3(int32_t down_sample_sblimit);
        static FMOD_RESULT makeTables(int32_t scaleval);
        static void dct12(float* in, float* rawout1, float* rawout2, float* wi, float* ts);
        static void dct36(float* inbuf, float* o1, float* o2, float* wintab, float* tsbuf);
        static void dct64(float* out0, float* out1, float* samples);

        FMOD_RESULT openInternal(FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        FMOD_RESULT readInternal(void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        FMOD_RESULT setPositionInternal(int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);
        FMOD_RESULT soundCreateInternal(int32_t subsound, FMOD_SOUND* sound);
        FMOD_RESULT resetInternal();

        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK openCallback(FMOD_CODEC_STATE* codec, FMOD_MODE usermode, FMOD_CREATESOUNDEXINFO* userexinfo);
        static FMOD_RESULT F_CALLBACK readCallback(FMOD_CODEC_STATE* codec, void* buffer, uint32_t sizebytes, uint32_t* bytesread);
        static FMOD_RESULT F_CALLBACK resetCallback(FMOD_CODEC_STATE* codec);
        static FMOD_RESULT F_CALLBACK setPositionCallback(FMOD_CODEC_STATE* codec, int32_t subsound, uint32_t position, FMOD_TIMEUNIT postype);
        static FMOD_RESULT F_CALLBACK soundCreateCallback(FMOD_CODEC_STATE* codec, int32_t subsound, FMOD_SOUND* sound);
        static FMOD_CODEC_DESCRIPTION_EX* getDescriptionEx();

        static bool gInitialized;
        static int32_t gTabSel123[2][3][16];
        static int32_t gFreqs[9];
        static uint8_t gGrp3Tab[96];
        static uint8_t gGrp5Tab[384];
        static uint8_t gGrp9Tab[3072];
        static float gMulsTab[27][64];
        static al_table gAlloc0[];
        static al_table gAlloc1[];
        static al_table gAlloc2[];
        static al_table gAlloc3[];
        static al_table gAlloc4[];
        static float gDecWinMem[560];
        static float gCos64[16];
        static float gCos32[8];
        static float gCos16[4];
        static float gCos8[2];
        static float gCos4[1];
        static float* gPnts[5];
        static int32_t gIntWinBase[257];

        static float gIsPowTable[8207];
        static float gAaCa[8];
        static float gAaCs[8];
        static float gWin[4][36];
        static float gWin1[4][36];
        static float gGainPow2[378];
        static float gCos6_1;
        static float gCos6_2;
        static float gTfCos36[9];
        static float gTfCos12[3];
        static float gCos9[3];
        static float gCos18[3];
        static int32_t gLongLimit[9][23];
        static int32_t gShortLimit[9][14];
        static bandInfoStruct gBandInfo[9];

        static int32_t gMapBuf0[9][152];
        static int32_t gMapBuf1[9][156];
        static int32_t gMapBuf2[9][44];
        static int32_t* gMap[9][3];
        static int32_t* gMapEnd[9][3];
        static uint32_t gN_SLen2[512];
        static uint32_t gI_SLen2[256];
        static float gTan1_1[16];
        static float gTan2_1[16];
        static float gTan1_2[16];
        static float gTan2_2[16];
        static float gPow1_1[2][16];
        static float gPow2_1[2][16];
        static float gPow1_2[2][16];
        static float gPow2_2[2][16];
};

} // namespace FMOD

#endif
