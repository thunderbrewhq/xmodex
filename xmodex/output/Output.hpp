#ifndef XMODEX_OUTPUT_OUTPUT_HPP
#define XMODEX_OUTPUT_OUTPUT_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"
#include "xmodex/Plugin.hpp"
#include "xmodex/dsp/DSPResampler.hpp"
#include "xmodex/os/Library.hpp"

#define FMOD_OUTPUT_MAXDRIVERS 32

namespace FMOD {

static const FMOD_OUTPUTTYPE FMOD_OUTPUTTYPE_EMULATED = static_cast<FMOD_OUTPUTTYPE>(-1);
static const FMOD_OUTPUTTYPE FMOD_OUTPUTTYPE_SOFTWARE = static_cast<FMOD_OUTPUTTYPE>(-2);

class Sample;
class ChannelPool;
class ChannelGroupI;
class SoundI;
class ChannelReal;

typedef int32_t(F_CALLBACK* FMOD_OUTPUT_GETSAMPLEMAXCHANNELS)(
    FMOD_OUTPUT_STATE* output,
    FMOD_MODE mode,
    FMOD_SOUND_FORMAT format);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETDRIVERINFOCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    char* name,
    int32_t namelen);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETDRIVERINFOWCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    int16_t* name,
    int32_t namelen);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETDRIVERCAPSEXCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    FMOD_CAPS* caps,
    int32_t* minfrequency,
    int32_t* maxfrequency,
    FMOD_SPEAKERMODE* controlpanelspeakermode);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETDRIVERCAPSEX2CALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    FMOD_CAPS* caps,
    int32_t* minfrequency,
    int32_t* maxfrequency,
    FMOD_SPEAKERMODE* controlpanelspeakermode,
    int32_t* num2dchannels,
    int32_t* num3dchannels,
    int32_t* totalchannels);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_INITEXCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t selecteddriver,
    FMOD_INITFLAGS flags,
    int32_t* outputrate,
    int32_t outputchannels,
    FMOD_SOUND_FORMAT* outputformat,
    FMOD_SPEAKERMODE* speakermode,
    int32_t dspbufferlength,
    int32_t dspnumbuffers,
    int32_t max2dchannels,
    int32_t max3dchannels,
    void* extradriverdata);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_STARTCALLBACK)(FMOD_OUTPUT_STATE* output);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_STOPCALLBACK)(FMOD_OUTPUT_STATE* output);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_UPDATEFINISHEDCALLBACK)(FMOD_OUTPUT_STATE* output);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_CREATESAMPLECALLBACK)(
    FMOD_OUTPUT_STATE* output,
    FMOD_MODE mode,
    FMOD_CODEC_WAVEFORMAT* waveformat,
    Sample** sample);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETSOUNDRAMCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t* currentalloced,
    int32_t* maxalloced,
    int32_t* total);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_POSTMIXCALLBACK)(FMOD_OUTPUT_STATE* output);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_RECORDGETDRIVERCAPSCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    FMOD_CAPS* caps,
    int32_t* minfrequency,
    int32_t* maxfrequency);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_RECORDSTARTCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    FMOD_SOUND* sound,
    int32_t loop);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_RECORDSTOPCALLBACK)(FMOD_OUTPUT_STATE* output);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_RECORDGETPOSITIONCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    uint32_t* pcm);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_RECORDLOCKCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    uint32_t offset,
    uint32_t length,
    void** ptr1,
    void** ptr2,
    uint32_t* len1,
    uint32_t* len2);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_RECORDUNLOCKCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    void* ptr1,
    void* ptr2,
    uint32_t len1,
    uint32_t len2);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_SETREVERBCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    FMOD_REVERB_PROPERTIES* prop);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETREVERBCALLBACK)(
    FMOD_OUTPUT_STATE* output,
    FMOD_REVERB_PROPERTIES* prop);

class FMOD_OUTPUT_DESCRIPTION_EX : public FMOD_OUTPUT_DESCRIPTION, public LinkedListNode {
    public:
        FMOD_OUTPUTTYPE mType;
        int32_t mSize;
        FMOD_OS_LIBRARY* mModule;
        FMOD_OUTPUT_GETSAMPLEMAXCHANNELS getsamplemaxchannels;
        FMOD_OUTPUT_GETDRIVERCAPSEXCALLBACK getdrivercapsex;
        FMOD_OUTPUT_GETDRIVERCAPSEX2CALLBACK getdrivercapsex2;
        FMOD_OUTPUT_INITEXCALLBACK initex;
        FMOD_OUTPUT_STARTCALLBACK start;
        FMOD_OUTPUT_STOPCALLBACK stop;
        FMOD_OUTPUT_UPDATEFINISHEDCALLBACK updatedfinished;
        FMOD_OUTPUT_CREATESAMPLECALLBACK createsample;
        FMOD_OUTPUT_GETSOUNDRAMCALLBACK getsoundram;
        FMOD_OUTPUT_POSTMIXCALLBACK postmixcallback;
        FMOD_OUTPUT_GETNUMDRIVERSCALLBACK record_getnumdrivers;
        FMOD_OUTPUT_GETDRIVERINFOCALLBACK record_getdriverinfo;
        FMOD_OUTPUT_RECORDGETDRIVERCAPSCALLBACK record_getdrivercaps;
        FMOD_OUTPUT_RECORDSTARTCALLBACK record_start;
        FMOD_OUTPUT_RECORDSTOPCALLBACK record_stop;
        FMOD_OUTPUT_RECORDGETPOSITIONCALLBACK record_getposition;
        FMOD_OUTPUT_RECORDLOCKCALLBACK record_lock;
        FMOD_OUTPUT_RECORDUNLOCKCALLBACK record_unlock;
        FMOD_OUTPUT_SETREVERBCALLBACK reverb_setproperties;
};

class Output : public Plugin, public FMOD_OUTPUT_STATE {
        friend class SystemI;
        friend class PluginFactory;
        friend class SoundI;
        friend class ChannelGroupI;

    public:
        ChannelPool* mChannelPool;

    protected:
        bool mPolling;
        SystemI* mSystem;
        ChannelPool* mChannelPool3D;
        int32_t mMixAheadBlocks;
        ChannelGroupI* mMusicChannelGroup;
        int32_t mNum2DChannelsFromCaps;
        int32_t mNum3DChannelsFromCaps;
        int32_t mTotalChannelsFromCaps;
        bool mRecording;
        int32_t mRecordOffset;
        bool mRecordLoop;
        FMOD_SOUND_FORMAT mRecordFormat;
        uint32_t mRecordLastCursorPos;
        uint32_t mRecordBufferLength;
        SoundI* mRecordSound;
        int32_t mRecordRate;
        bool mRecordResample;
        DSPResampler mRecordResamplerDSP;
        float* mRecordTempBuffer;
        uint32_t mRecordTempBufferLength;

    public:
        FMOD_OUTPUT_DESCRIPTION_EX mDescription;

    public:
        Output();

        FMOD_RESULT release();
        FMOD_RESULT getFreeChannel(FMOD_MODE mode, ChannelReal** realchannel, int32_t numchannels, int32_t numsoundchannels, int32_t* found);
        FMOD_RESULT recordRead(float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        FMOD_RESULT recordFill(uint32_t length);
        FMOD_RESULT recordUpdate();
        FMOD_RESULT mix(void* buffer, uint32_t numsamples);

        static FMOD_RESULT F_CALLBACK recordResamplerReadCallback(FMOD_DSP_STATE* dsp_state, float* inbuffer, float* outbuffer, uint32_t length, int32_t inchannels, int32_t outchannels);
        static FMOD_RESULT F_CALLBACK mixCallback(FMOD_OUTPUT_STATE* output, void* buffer, uint32_t length);
};

} // namespace FMOD

#endif
