#ifndef XMODEX_OUTPUT_WIN_OUTPUT_WASAPI_HPP
#define XMODEX_OUTPUT_WIN_OUTPUT_WASAPI_HPP

#include "xmodex/output/win/OutputTimer.hpp"
#include <initguid.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <cstdint>

namespace FMOD {

struct FMOD_WASAPIDriver {
        LPWSTR id;
        char* name;
};

class OutputWASAPI : public OutputTimer {
    private:
        bool mCoInitialized;
        bool mInitialised;
        bool mExclusiveMode;
        int32_t mNumRenderDrivers;
        FMOD_WASAPIDriver mRenderDrivers[32];
        IAudioClient* mRenderAudioClient;
        IAudioClock* mRenderAudioClock;
        IAudioRenderClient* mRenderClient;
        uint8_t* mRenderBuffer;
        uint32_t mRenderBufferLength;
        int32_t mNumCaptureDrivers;
        FMOD_WASAPIDriver mCaptureDrivers[32];
        IAudioClient* mCaptureAudioClient;
        IAudioCaptureClient* mCaptureClient;
        uint8_t* mRecordBuffer;
        int32_t mRecordBlockAlign;
        float mRecordTimerPeriod;
        int32_t mRecordWritePosition;
        bool mRecordStereoToMono;
        FMOD_OS_CRITICALSECTION* mRecordCrit;
        uint32_t mRecordTimerID;

    private:
        FMOD_RESULT close();
        FMOD_RESULT enumerate();
        FMOD_RESULT enumerateDefaultDevices(IMMDeviceEnumerator* pEnumerator);
        FMOD_RESULT getDriverCapsEx(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode);
        FMOD_RESULT getDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT getHandle(void** handle);
        FMOD_RESULT getNumDrivers(int32_t* numdrivers);
        FMOD_RESULT getPosition(uint32_t* pcm);
        FMOD_RESULT initEx(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, FMOD_SPEAKERMODE* speakermode, int32_t dspbufferlength, int32_t dspnumbuffers, int32_t max2dchannels, int32_t max3dchannels, void* extradriverdata);
        FMOD_RESULT lock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2);
        FMOD_RESULT recordGetDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency);
        FMOD_RESULT recordGetDriverInfo(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT recordGetNumDrivers(int32_t* numdrivers);
        FMOD_RESULT recordLock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2);
        FMOD_RESULT recordStart(int32_t id, Sound* sound, bool loop);
        FMOD_RESULT recordStop();
        FMOD_RESULT recordUpdate();
        FMOD_RESULT start();
        FMOD_RESULT stop();
        FMOD_RESULT unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2);

        static void F_CALLBACK recordUpdateCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

    public:
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK getDriverCapsExCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode);
        static FMOD_RESULT F_CALLBACK getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen);
        static FMOD_RESULT F_CALLBACK getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle);
        static FMOD_RESULT F_CALLBACK getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK getPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm);
        static FMOD_RESULT F_CALLBACK initExCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, FMOD_SPEAKERMODE* speakermode, int32_t dspbufferlength, int32_t dspnumbuffers, int32_t max2dchannels, int32_t max3dchannels, void* extradriverdata);
        static FMOD_RESULT F_CALLBACK lockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2);
        static FMOD_RESULT F_CALLBACK recordGetDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency);
        static FMOD_RESULT F_CALLBACK recordGetDriverInfoCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen);
        static FMOD_RESULT F_CALLBACK recordGetNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK recordGetPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm);
        static FMOD_RESULT F_CALLBACK recordLockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2);
        static FMOD_RESULT F_CALLBACK recordStartCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_SOUND* sound, int32_t loop);
        static FMOD_RESULT F_CALLBACK recordStopCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK startCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK stopCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK unlockCallback(FMOD_OUTPUT_STATE* output, void* ptr1, void* ptr2, uint32_t len1, uint32_t len2);

        static FMOD_OUTPUT_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
