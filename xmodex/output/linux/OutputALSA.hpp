#ifndef XMODEX_OUTPUT_LINUX_OUTPUT_ALSA_HPP
#define XMODEX_OUTPUT_LINUX_OUTPUT_ALSA_HPP

#include "xmodex/Thread.hpp"
#include "xmodex/output/Output.hpp"

struct snd_pcm_t;

namespace FMOD {

constexpr int32_t RECORD_MAXBLOCKS = 100;

// size = 0x6C0
class OutputALSA : public Output {
    private:
        bool mEnumerated;
        snd_pcm_t* mHandle;
        snd_pcm_t* mRecordHandle;
        void* mOutputBuffer;
        void* mRecordBuffer;
        void* mLibrary;
        int32_t mNumOutputChannels;
        int32_t mRecordChannels;
        int32_t mOutputRate;
        FMOD_SOUND_FORMAT mOutputFormat;
        uint32_t mRecordCurrentBlock;
        uint32_t mRecordBlockLenBytes;
        uint32_t mRecordBufferBytes;
        uint32_t mRecordPosition;
        Thread mMixerThread;
        Thread mRecordThread;
        int32_t mNumDrivers;
        int32_t mMaxDrivers;
        char** mDriverNames;

    private:
        FMOD_RESULT enumerate();
        FMOD_RESULT getALSAdrivers(const char* name);
        FMOD_RESULT registerLib();
        FMOD_RESULT updateMixer();
        FMOD_RESULT updateRecord();

    private:
        static void F_CALLBACK recordThreadCallback(void* data);
        static void F_CALLBACK mixThreadCallback(void* data);

    private:
        FMOD_RESULT getNumDrivers(int32_t* numdrivers);
        FMOD_RESULT getDriverName(
            int32_t id,
            char* name,
            int32_t namelen);
        FMOD_RESULT init(
            int32_t selecteddriver,
            FMOD_INITFLAGS flags,
            int32_t* outputrate,
            int32_t outputchannels,
            FMOD_SOUND_FORMAT* outputformat,
            int32_t dspbufferlength,
            int32_t dspnumbuffers,
            void* extradriverdata);
        FMOD_RESULT close();
        FMOD_RESULT start();
        FMOD_RESULT stop();
        FMOD_RESULT getHandle(void** handle);
        FMOD_RESULT recordGetNumDrivers(int32_t* numdrivers);
        FMOD_RESULT recordGetDriverInfo(
            int32_t id,
            char* name,
            int32_t namelen);
        FMOD_RESULT recordStart(
            int32_t id,
            FMOD_SOUND* sound,
            int32_t loop);
        FMOD_RESULT recordStop();
        FMOD_RESULT recordGetPosition(uint32_t* pcm);
        FMOD_RESULT recordLock(
            uint32_t offset,
            uint32_t length,
            void** ptr1,
            void** ptr2,
            uint32_t* len1,
            uint32_t* len2);

    public:
        static FMOD_RESULT F_CALLBACK getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK getDriverNameCallback(
            FMOD_OUTPUT_STATE* output,
            int32_t id,
            char* name,
            int32_t namelen);
        static FMOD_RESULT F_CALLBACK initCallback(
            FMOD_OUTPUT_STATE* output,
            int32_t selecteddriver,
            FMOD_INITFLAGS flags,
            int32_t* outputrate,
            int32_t outputchannels,
            FMOD_SOUND_FORMAT* outputformat,
            int32_t dspbufferlength,
            int32_t dspnumbuffers,
            void* extradriverdata);
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK startCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK stopCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle);
        static FMOD_RESULT F_CALLBACK recordGetNumDriversCallback(
            FMOD_OUTPUT_STATE* output,
            int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK recordGetDriverInfoCallback(
            FMOD_OUTPUT_STATE* output,
            int32_t id,
            char* name,
            int32_t namelen);
        static FMOD_RESULT F_CALLBACK recordStartCallback(
            FMOD_OUTPUT_STATE* output,
            int32_t id,
            FMOD_SOUND* sound,
            int32_t loop);
        static FMOD_RESULT F_CALLBACK recordStopCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK recordGetPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm);
        static FMOD_RESULT F_CALLBACK recordLockCallback(
            FMOD_OUTPUT_STATE* output,
            uint32_t offset,
            uint32_t length,
            void** ptr1,
            void** ptr2,
            uint32_t* len1,
            uint32_t* len2);

        static FMOD_OUTPUT_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
