#ifndef XMODEX_SYSTEM_I_HPP
#define XMODEX_SYSTEM_I_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"
#include "xmodex/Listener.hpp"
#include "xmodex/MemSingleton.hpp"
#include "xmodex/ReverbI.hpp"
#include "xmodex/SpeakerLevelsPool.hpp"
#include "xmodex/Thread.hpp"
#include "xmodex/TimeStamp.hpp"
#include "xmodex/Types.hpp"
#include "xmodex/channel/ChannelGroupI.hpp"
#include "xmodex/channel/ChannelI.hpp"
#include "xmodex/dsp/DSPCodecPool.hpp"
#include "xmodex/dsp/DSPConnectionPool.hpp"
#include "xmodex/dsp/DSPConnectionRequest.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/os/CriticalSection.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

class Output;
class OutputEmulated;
class OutputSoftware;
class PluginFactory;
class DSPI;
class FMOD_DSP_DESCRIPTION_EX;
class Sample;

constexpr int32_t FMOD_MAXAUDIBIILITY = 1000;
constexpr int32_t FMOD_MAXPRIORITIES = 256;

constexpr FMOD_SPEAKERMODE FMOD_SPEAKERMODE_STEREO_LINEAR = static_cast<FMOD_SPEAKERMODE>(1000);

struct FMOD_SPEAKERCONFIG {
        FMOD_SPEAKER mSpeaker;
        FMOD_VECTOR mPosition;
        uint32_t mXZAngle;
        float mDistance;
        bool mActive;
};

class SystemI : public LinkedListNode {
    public:
        bool mInitialized;
        bool mPluginsLoaded;
        FMOD_UINT_NATIVE mMainThreadID;
        FMOD_INITFLAGS mFlags;
        SoundI mSoundListHead;
        int32_t mNumChannels;
        ChannelI* mChannel;
        ChannelI mChannelUsedListHead;
        ChannelI mChannelFreeListHead;
        SortedLinkedListNode mChannelSortedListHead;
        Output* mOutput;
        FMOD_OUTPUTTYPE mOutputType;
        FMOD_SOUND_FORMAT mOutputFormat;
        int32_t mOutputRate;
        int32_t mOutputHandle;
        int32_t mMaxInputChannels;
        int32_t mMaxOutputChannels;
        int32_t mSelectedDriver;
        OutputEmulated* mEmulated;
        int32_t mRecordID;
        uint32_t mDSPBlockSize;
        uint32_t mDSPBufferSize;
        float* mDSPTempBuff[2];
        float* mDSPTempBuffMem[2];
        float* mDSPMixBuff[FMOD_DSP_MAXTREEDEPTH];
        DSPConnectionPool mDSPConnectionPool;
        FMOD_OS_CRITICALSECTION* mDSPCrit;
        FMOD_OS_CRITICALSECTION* mDSPLockCrit;
        FMOD_OS_CRITICALSECTION* mDSPConnectionCrit;
        FMOD_OS_CRITICALSECTION* mDSPRequestCrit;
        bool mDSPActive;
        DSPI* mDSPSoundCard;
        DSPI* mDSPChannelGroupTarget;
        TimeStamp mDSPTimeStamp;
        int32_t mDSPTempBuffIndex;
        DSPConnectionRequest mConnectionRequest[FMOD_DSP_CONNECTION_REQUEST_MAX];
        DSPConnectionRequest mConnectionRequestUsedHead;
        DSPConnectionRequest mConnectionRequestFreeHead;
        bool mConnectionRequestFlushing;
        Listener mListener[4];
        int32_t mNumListeners;
        float mDistanceScale;
        float mRolloffScale;
        float mDopplerScale;
        FMOD_3D_ROLLOFFCALLBACK mRolloffCallback;
        PluginFactory* mPluginFactory;
        char mPluginPath[FMOD_STRING_MAXPATHLEN];
        int32_t mFSBPluginHandle;
        int32_t mWAVPluginHandle;
        int32_t mMPEGPluginHandle;
        FMOD_ADVANCEDSETTINGS mAdvancedSettings;
        void* mUserData;
        TimeStamp mUpdateTimeStamp;
        uint32_t mLastTimeStamp;
        uint32_t mIndex;
        int32_t mNumSoftwareChannels;
        int32_t mMinHardwareChannels2D;
        int32_t mMaxHardwareChannels2D;
        int32_t mMinHardwareChannels3D;
        int32_t mMaxHardwareChannels3D;
        ChannelGroupI* mChannelGroup;
        ChannelGroupI mChannelGroupHead;
        SoundGroupI* mSoundGroup;
        LinkedListNode mSoundGroupUsedHead;
        LinkedListNode mSoundGroupFreeHead;
        FMOD_DSP_RESAMPLER mResampleMethod;
        MemSingleton mMultiSubSampleLockBuffer;
        FMOD_OS_CRITICALSECTION* mMultiSubSampleLockBufferCrit;
        FMOD_OS_CRITICALSECTION* mReadDataCrit;
        FMOD_FILE_OPENCALLBACK mOpenRiderCallback;
        FMOD_FILE_CLOSECALLBACK mCloseRiderCallback;
        FMOD_FILE_READCALLBACK mReadRiderCallback;
        FMOD_FILE_SEEKCALLBACK mSeekRiderCallback;
        FMOD_SYSTEM_CALLBACK mCallback[FMOD_SYSTEM_CALLBACKTYPE_MAX];
        SpeakerLevelsPool mSpeakerLevelsPool;
        uint32_t mStreamFileBufferSize;
        FMOD_TIMEUNIT mStreamFileBufferSizeType;
        FMOD_SPEAKERMODE mSpeakerMode;
        FMOD_SPEAKERCONFIG mSpeaker[FMOD_SPEAKER_MAX];
        FMOD_SPEAKERCONFIG* mSpeakerList[FMOD_SPEAKER_MAX];
        LinkedListNode mStreamListChannelHead;
        LinkedListNode* mStreamListChannelCurrent;
        LinkedListNode* mStreamListChannelNext;
        LinkedListNode mStreamListSoundHead;
        Thread mStreamThread;
        bool mStreamThreadActive;
        FMOD_OS_CRITICALSECTION* mStreamRealchanCrit;
        FMOD_OS_CRITICALSECTION* mStreamUpdateCrit;
        FMOD_OS_CRITICALSECTION* mStreamListCrit;
        TimeStamp mStreamTimeStamp;
        OutputSoftware* mSoftware;
        FMOD_SPEAKER mASIOSpeakerList[16];
        DSPCodecPool mDSPCodecPool_MPEG;
        DSPCodecPool mDSPCodecPool_ADPCM;
        // GeometryI* mGeometryList;
        // GeometryMgr mGeometryMgr;
        ReverbI mReverbGlobal;
        ReverbI mReverb3D;
        ReverbI mReverb3DHead;
        bool mReverb3DActive;
        FMOD_REVERB_PROPERTIES mReverb3DAmbientProperties;
        // DSPNet mDSPNet;
    public:
        SystemI();

        FMOD_RESULT allocDSPCodec(FMOD_SOUND_FORMAT format, DSPI** dsp);
        FMOD_RESULT checkDriverList(bool fromsystemupdate);
        FMOD_RESULT close();
        FMOD_RESULT closeEx(bool calledfrominit);
        FMOD_RESULT createChannelGroup(const char* name, ChannelGroupI** channelgroup);
        FMOD_RESULT createDSP(FMOD_DSP_DESCRIPTION* description, DSPI** dsp);
        FMOD_RESULT createDSP(FMOD_DSP_DESCRIPTION_EX* description, DSPI** dsp, bool allocate);
        FMOD_RESULT createDSPByType(FMOD_DSP_TYPE type, DSPI** dsp);
        FMOD_RESULT createSample(FMOD_MODE mode, FMOD_CODEC_WAVEFORMAT* waveformat, Sample** sample_out);
        FMOD_RESULT createSound(const char* name_or_data, FMOD_MODE mode_in, FMOD_CREATESOUNDEXINFO* exinfo, SoundI** sound);
        FMOD_RESULT createSoundGroup(const char* name, SoundGroupI** soundgroup);
        FMOD_RESULT createSoundInternal(const char* name_or_data, FMOD_MODE mode_in, FMOD_CREATESOUNDEXINFO* exinfo, bool calledfromasync, SoundI** sound);
        FMOD_RESULT createStream(const char* name_or_data, FMOD_MODE mode_in, FMOD_CREATESOUNDEXINFO* exinfo, SoundI** sound);
        FMOD_RESULT findChannel(FMOD_CHANNELINDEX id, DSPI* dsp, ChannelI** channel);
        FMOD_RESULT findChannel(FMOD_CHANNELINDEX id, SoundI* sound, ChannelI** channel);
        FMOD_RESULT flushDSPConnectionRequests(bool calledfrommainthread);
        FMOD_RESULT get3DListenerAttributes(int32_t listener, FMOD_VECTOR* pos, FMOD_VECTOR* vel, FMOD_VECTOR* forward, FMOD_VECTOR* up);
        FMOD_RESULT get3DNumListeners(int32_t* numlisteners);
        FMOD_RESULT get3DReverbActive(bool* state);
        FMOD_RESULT get3DSettings(float* dopplerscale, float* distancescale, float* rolloffscale);
        FMOD_RESULT getChannel(int32_t id, ChannelI** channel);
        FMOD_RESULT getDSPBufferSize(uint32_t* bufferlength, int32_t* numbuffers);
        FMOD_RESULT getDSPHead(DSPI** dsp);
        FMOD_RESULT getDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode);
        FMOD_RESULT getDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT getHardwareChannels(int32_t* num2d, int32_t* num3d, int32_t* total);
        FMOD_RESULT getListenerObject(int32_t listener, Listener** listenerobject);
        FMOD_RESULT getMasterChannelGroup(ChannelGroupI** channelgroup);
        FMOD_RESULT getNumDrivers(int32_t* numdrivers);
        FMOD_RESULT getOutput(FMOD_OUTPUTTYPE* output);
        FMOD_RESULT getRecordDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT getRecordNumDrivers(int32_t* numdrivers);
        FMOD_RESULT getRecordPosition(uint32_t* position);
        FMOD_RESULT getReverbAmbientProperties(FMOD_REVERB_PROPERTIES* prop);
        FMOD_RESULT getSoftwareFormat(int32_t* samplerate, FMOD_SOUND_FORMAT* format, int32_t* numoutputchannels, int32_t* maxinputchannels, FMOD_DSP_RESAMPLER* resamplemethod, int32_t* bits);
        FMOD_RESULT getSpeakerMode(FMOD_SPEAKERMODE* speakermode);
        FMOD_RESULT getVersion(uint32_t* version);
        FMOD_RESULT init(int32_t maxchannels, FMOD_INITFLAGS flags, void* extradriverdata);
        FMOD_RESULT isRecording(bool* recording);
        FMOD_RESULT lockDSP();
        FMOD_RESULT playDSP(FMOD_CHANNELINDEX channelid, DSPI* dsp, bool paused, ChannelI** channel);
        FMOD_RESULT playSound(FMOD_CHANNELINDEX channelid, SoundI* sound, bool paused, ChannelI** channel);
        FMOD_RESULT recordStart(SoundI* sound, bool loop);
        FMOD_RESULT recordStop();
        FMOD_RESULT release();
        FMOD_RESULT set3DListenerAttributes(int32_t listener, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel, const FMOD_VECTOR* forward, const FMOD_VECTOR* up);
        FMOD_RESULT set3DReverbActive(bool state);
        FMOD_RESULT set3DReverbProperties(FMOD_REVERB_PROPERTIES* prop, bool force_create);
        FMOD_RESULT set3DRolloffCallback(FMOD_3D_ROLLOFFCALLBACK callback);
        FMOD_RESULT set3DSettings(float dopplerscale, float distancescale, float rolloffscale);
        FMOD_RESULT set3DSpeakerPosition(FMOD_SPEAKER speaker, float x, float y, bool active);
        FMOD_RESULT setCallback(FMOD_SYSTEM_CALLBACKTYPE type, FMOD_SYSTEM_CALLBACK callback);
        FMOD_RESULT setDSPBufferSize(uint32_t bufferlength, int32_t numbuffers);
        FMOD_RESULT setDriver(int32_t driver);
        FMOD_RESULT setHardwareChannels(int32_t min2d, int32_t max2d, int32_t min3d, int32_t max3d);
        FMOD_RESULT setOutput(FMOD_OUTPUTTYPE outputtype);
        FMOD_RESULT setRecordDriver(int32_t recorddriver);
        FMOD_RESULT setReverbAmbientProperties(FMOD_REVERB_PROPERTIES* prop);
        FMOD_RESULT setReverbProperties(FMOD_REVERB_PROPERTIES* prop, bool force_create);
        FMOD_RESULT setSoftwareChannels(int32_t numsoftwarechannels);
        FMOD_RESULT setSoftwareFormat(int32_t samplerate, FMOD_SOUND_FORMAT format, int32_t numoutputchannels, int32_t maxinputchannels, FMOD_DSP_RESAMPLER resamplermethod);
        FMOD_RESULT setSpeakerMode(FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT setUpPlugins();
        FMOD_RESULT sortSpeakerList();
        FMOD_RESULT stopDSP(DSPI* dsp);
        FMOD_RESULT stopSound(SoundI* sound);
        FMOD_RESULT unlockDSP();
        FMOD_RESULT update();
        FMOD_RESULT update3DReverbs();
        FMOD_RESULT updateChannels(int32_t delta);
        FMOD_RESULT updateSoundGroups(int32_t delta);
        FMOD_RESULT updateStreams();
        uint32_t count3DPhysicalReverbs();
        uint32_t count3DVirtualReverbs();

        static FMOD_RESULT getInstance(uint32_t id, SystemI** sys);
        static FMOD_RESULT validate(System* system, SystemI** systemi);
        static void streamThread(void* data);

    public:
        static FMOD_OS_CRITICALSECTION* gSoundListCrit;
};

}; // namespace FMOD

#endif
