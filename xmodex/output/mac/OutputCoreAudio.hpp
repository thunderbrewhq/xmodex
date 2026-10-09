#ifndef XMODEX_OUTPUT_MAC_OUTPUT_CORE_AUDIO_HPP
#define XMODEX_OUTPUT_MAC_OUTPUT_CORE_AUDIO_HPP

#include "xmodex/output/Output.hpp"
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

namespace FMOD {

struct CoreAudioDevice {
        AudioDeviceID id;
        char* name;
};

class OutputCoreAudio : public Output {
    private:
        bool mInitialized;
        int32_t mNumOutputDevices;
        CoreAudioDevice mOutputDevices[32];
        AudioUnit mOutputUnit;
        int32_t mNumInputDevices;
        CoreAudioDevice mInputDevices[32];
        AudioUnit mInputUnit;
        void* mRecordBuffer;
        AudioBufferList mRecordBufferList;
        uint32_t mRecordNumBytesPerFrame;
        uint32_t mRecordMaxFramesPerSlice;
        uint32_t mRecordSliceIndex;

    private:
        static OSStatus renderProc(
            void* inrefcon,
            AudioUnitRenderActionFlags* ioactionflags,
            const AudioTimeStamp* intimestamp,
            UInt32 inbusnumber,
            UInt32 innumberframes,
            AudioBufferList* iodata);
        static OSStatus recordProc(
            void* inrefcon,
            AudioUnitRenderActionFlags* ioactionflags,
            const AudioTimeStamp* intimestamp,
            UInt32 inbusnumber,
            UInt32 innumberframes,
            AudioBufferList* iodata);

    private:
        FMOD_RESULT enumerate();
        FMOD_RESULT enumerateDefaultDevices();

    private:
        FMOD_RESULT getNumDrivers(int32_t* numdrivers);
        FMOD_RESULT getDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT getDriverCapsEx(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode);
        FMOD_RESULT init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata);
        FMOD_RESULT close();
        FMOD_RESULT start();
        FMOD_RESULT stop();
        FMOD_RESULT getHandle(void** handle);
        FMOD_RESULT recordGetNumDrivers(int32_t* numdrivers);
        FMOD_RESULT recordGetDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT recordGetDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency);
        FMOD_RESULT recordStart(int32_t id, FMOD_SOUND* sound, int32_t loop);
        FMOD_RESULT recordStop();
        FMOD_RESULT recordGetPosition(uint32_t* pcm);
        FMOD_RESULT recordLock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2);

    public:
        static FMOD_RESULT F_CALLBACK getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen);
        static FMOD_RESULT F_CALLBACK getDriverCapsExCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode);
        static FMOD_RESULT F_CALLBACK initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata);
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK startCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK stopCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle);
        static FMOD_RESULT F_CALLBACK recordGetNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK recordGetDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen);
        static FMOD_RESULT F_CALLBACK recordGetDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency);
        static FMOD_RESULT F_CALLBACK recordStartCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_SOUND* sound, int32_t loop);
        static FMOD_RESULT F_CALLBACK recordStopCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK recordGetPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm);
        static FMOD_RESULT F_CALLBACK recordLockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2);

        static FMOD_OUTPUT_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
