#ifndef XMODEX_DSP_DSPI_HPP
#define XMODEX_DSP_DSPI_HPP

#define DSP_MAXLEVELS_IN 16
#define DSP_MAXLEVELS_OUT 16

#define FMOD_DSP_MAXTREEDEPTH 128

#define FMOD_DSP_FLAG_VISITED 0x00000001
#define FMOD_DSP_FLAG_ACTIVE 0x00000002
#define FMOD_DSP_FLAG_BYPASS 0x00000004
#define FMOD_DSP_FLAG_FINISHED 0x00000008
#define FMOD_DSP_FLAG_IDLE 0x00000010
#define FMOD_DSP_FLAG_USEDADDDSP 0x00000020

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"
#include "xmodex/Plugin.hpp"
#include "xmodex/os/Library.hpp"

namespace FMOD {

struct DSPConnection;

enum FMOD_DSP_CATEGORY {
    FMOD_DSP_CATEGORY_FILTER,
    FMOD_DSP_CATEGORY_DSPCODECMPEG,
    FMOD_DSP_CATEGORY_DSPCODECADPCM,
    FMOD_DSP_CATEGORY_DSPCODECXMA,
    FMOD_DSP_CATEGORY_DSPCODECRAW,
    FMOD_DSP_CATEGORY_SOUNDCARD,
    FMOD_DSP_CATEGORY_WAVETABLE,
    FMOD_DSP_CATEGORY_RESAMPLER
};

enum FMOD_DSP_LOWPASS_SIMPLE {
    FMOD_DSP_LOWPASS_SIMPLE_CUTOFF
};

constexpr int32_t FMOD_DSP_TYPE_CODECREADER = 1000;

class FMOD_DSP_DESCRIPTION_EX : public FMOD_DSP_DESCRIPTION, public LinkedListNode {
    public:
        FMOD_SOUND_FORMAT mFormat;
        FMOD_DSP_TYPE mType;
        int32_t mSize;
        FMOD_DSP_CATEGORY mCategory;
        FMOD_OS_LIBRARY* mModule;
        void* mAEffect;
        int32_t mResamplerBlockLength;
};

class DSPI : public Plugin, public FMOD_DSP_STATE {
        friend class DSPFilter;

    private:
        LinkedListNode mInputHead;
        LinkedListNode mOutputHead;
        int32_t mNumInputs;
        int32_t mNumOutputs;

    protected:
        float* mOutputBuffer;
        int32_t mTargetFrequency;
        int32_t mTreeLevel;

    public:
        FMOD_DSP_DESCRIPTION_EX mDescription;
        float mDefaultVolume;
        float mDefaultFrequency;
        float mDefaultPan;
        int32_t mDefaultPriority;
        float* mBuffer;
        int32_t mBufferChannels;
        uint32_t mFlags;
        uint8_t* mWantsToFinish;
        uint8_t mWantsToFinishMem;

    public:
        DSPI();

        FMOD_RESULT resetVisited();
        FMOD_RESULT updateTreeLevel(int32_t level);
        FMOD_RESULT getNumInputs(int32_t* numinputs);
        FMOD_RESULT getNumOutputs(int32_t* numoutputs);
        FMOD_RESULT setUserData(void* userdata);
        FMOD_RESULT getUserData(void** userdata);
        FMOD_RESULT setFinished(bool finished, bool force);
        FMOD_RESULT getInput(int32_t index, DSPConnection** input, DSPI** inputdsp);
        FMOD_RESULT getOutput(int32_t index, DSPConnection** output, DSPI** outputdsp);
        FMOD_RESULT disconnectFromInternal(DSPI* target);
        FMOD_RESULT disconnectFrom(DSPI* dsp);
        FMOD_RESULT disconnectFromQueued(DSPI* target);
        FMOD_RESULT disconnectAll(bool inputs, bool outputs);
        FMOD_RESULT getInput(int32_t index, DSPI** inputdsp);
        FMOD_RESULT getOutput(int32_t index, DSPI** outputdsp);
        FMOD_RESULT doesUnitExist(DSPI* target);
        FMOD_RESULT addInputInternal(
            DSPI* target,
            bool checkcircular,
            DSPConnection* connection,
            DSPConnection** connection_out);
        FMOD_RESULT addInputQueued(
            DSPI* target,
            bool checkcircular,
            DSPConnection* connection_old,
            DSPConnection** connection_out);
        FMOD_RESULT disconnectAllInternal(bool inputs, bool outputs);
        FMOD_RESULT disconnectAllQueued(bool inputs, bool outputs);

        inline FMOD_RESULT setActive(bool active) {
            if (active) {
                this->mFlags |= FMOD_DSP_FLAG_ACTIVE;
            } else {
                this->mFlags &= ~FMOD_DSP_FLAG_ACTIVE;
            }
            return FMOD_OK;
        }

        inline FMOD_RESULT getActive(bool* active) {
            *active = this->mFlags & FMOD_DSP_FLAG_ACTIVE ? true : false;
            return FMOD_OK;
        }

        inline FMOD_RESULT setBypass(bool bypass) {
            if (bypass) {
                this->mFlags |= FMOD_DSP_FLAG_BYPASS;
            } else {
                this->mFlags &= ~FMOD_DSP_FLAG_BYPASS;
            }

            return FMOD_OK;
        }

        inline FMOD_RESULT getBypass(bool* bypass) {
            *bypass = this->mFlags & FMOD_DSP_FLAG_BYPASS ? true : false;
            return FMOD_OK;
        }

        inline FMOD_RESULT getFinished(bool* finished) {
            if (!*this->mWantsToFinish) {
                *finished = this->mFlags & FMOD_DSP_FLAG_FINISHED ? true : false;
            } else {
                *finished = false;
            }

            return FMOD_OK;
        }

        virtual FMOD_RESULT execute(
            float* inbuffer,
            float** outbuffer,
            uint32_t* length,
            int32_t inchannels,
            int32_t* outchannels,
            FMOD_SPEAKERMODE speakermode);
        virtual FMOD_RESULT execute(
            void* inbuffer,
            void** outbuffer,
            uint32_t* length,
            int32_t inchannels,
            int32_t* outchannels,
            FMOD_SPEAKERMODE speakermode);
        virtual FMOD_RESULT alloc(FMOD_DSP_DESCRIPTION_EX* description);
        virtual FMOD_RESULT setPosition(uint32_t position, bool processinputs);
        virtual FMOD_RESULT getSystemObject(SystemI** system);
        virtual FMOD_RESULT addInput(DSPI* target);
        virtual FMOD_RESULT release(bool freethis);
        virtual FMOD_RESULT remove();
        virtual FMOD_RESULT reset();
        virtual FMOD_RESULT setParameter(int32_t index, float value);
        virtual FMOD_RESULT getParameter(
            int32_t index,
            float* value,
            char* valuestr,
            int32_t valuestrlen);
        virtual FMOD_RESULT getNumParameters(int32_t* numparams);
        virtual FMOD_RESULT getParameterInfo(
            int32_t index,
            char* name,
            char* label,
            char* description,
            int32_t descriptionlen,
            float* min,
            float* max);
        virtual FMOD_RESULT showConfigDialog(void* hwnd, bool show);
        virtual FMOD_RESULT getInfo(
            char* name,
            uint32_t* version,
            int32_t* channels,
            int32_t* configwidth,
            int32_t* configheight);
        virtual FMOD_RESULT getType(FMOD_DSP_TYPE* type);
        virtual FMOD_RESULT setDefaults(
            float frequency,
            float volume,
            float pan,
            int32_t priority);
        virtual FMOD_RESULT getDefaults(
            float* frequency,
            float* volume,
            float* pan,
            int32_t* priority);
        virtual FMOD_RESULT setTargetFrequency(int32_t frequency);
        virtual FMOD_RESULT getTargetFrequency(int32_t* frequency);

        static FMOD_RESULT validate(DSP* dsp, DSPI** dspi);
        static FMOD_RESULT calculateSpeakerLevels(
            float frontleft,
            float frontright,
            float center,
            float lfe,
            float backleft,
            float backright,
            float sideleft,
            float sideright,
            FMOD_SPEAKERMODE speakermode,
            int32_t channels,
            FMOD_SPEAKERMAPTYPE speakermap,
            float* outlevels,
            int32_t* numinputlevels);
        static FMOD_RESULT convert(
            void* outbuffer,
            void* inbuffer,
            FMOD_SOUND_FORMAT outformat,
            FMOD_SOUND_FORMAT informat,
            uint32_t length,
            int32_t destchannelstep,
            int32_t srcchannelstep,
            float volume);
};

} // namespace FMOD

#endif
