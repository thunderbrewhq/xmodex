#ifndef XMODEX_OUTPUT_OUTPUT_WAV_WRITER_NRT_HPP
#define XMODEX_OUTPUT_OUTPUT_WAV_WRITER_NRT_HPP

#include "xmodex/String.hpp"
#include "xmodex/output/Output.hpp"
#include <cstdio>

namespace FMOD {

class OutputWavWriter_NRT : public Output {
    private:
        char mFileName[FMOD_STRING_MAXPATHLEN];
        void* mBuffer;
        uint32_t mBufferLength;
        uint32_t mBufferLengthBytes;
        int32_t mChannels;
        int32_t mRate;
        int32_t mBits;
        uint32_t mLengthBytes;
        FMOD_SOUND_FORMAT mFormat;
        FILE* mFP;

    private:
        FMOD_RESULT writeWavHeader();

    private:
        FMOD_RESULT getNumDrivers(int32_t* numdrivers);
        FMOD_RESULT getDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT getDriverCaps(int32_t id, FMOD_CAPS* caps);
        FMOD_RESULT init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata);
        FMOD_RESULT close();
        FMOD_RESULT getHandle(void** handle);
        FMOD_RESULT update();
        FMOD_RESULT start();
        FMOD_RESULT stop();

    public:
        static FMOD_RESULT F_CALLBACK getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen);
        static FMOD_RESULT F_CALLBACK getDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps);
        static FMOD_RESULT F_CALLBACK initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata);
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK startCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK stopCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK updateCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle);

        static FMOD_OUTPUT_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
