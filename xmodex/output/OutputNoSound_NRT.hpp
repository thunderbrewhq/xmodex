#ifndef XMODEX_OUTPUT_OUTPUT_NO_SOUND_NRT_HPP
#define XMODEX_OUTPUT_OUTPUT_NO_SOUND_NRT_HPP

#include "xmodex/output/Output.hpp"

namespace FMOD {

class OutputNoSound_NRT : public Output {
    private:
        uint32_t mBufferLength;
        void* mBuffer;

    public:
        FMOD_RESULT getNumDrivers(int32_t* numdrivers);
        FMOD_RESULT getDriverName(int32_t driver, char* name, int32_t namelen);
        FMOD_RESULT getDriverCaps(int32_t id, FMOD_CAPS* caps);
        FMOD_RESULT init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata);
        FMOD_RESULT close();
        FMOD_RESULT update();

        static FMOD_RESULT F_CALLBACK getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers);
        static FMOD_RESULT F_CALLBACK getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen);
        static FMOD_RESULT F_CALLBACK getDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps);
        static FMOD_RESULT F_CALLBACK initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata);
        static FMOD_RESULT F_CALLBACK closeCallback(FMOD_OUTPUT_STATE* output);
        static FMOD_RESULT F_CALLBACK updateCallback(FMOD_OUTPUT_STATE* output);

        static FMOD_OUTPUT_DESCRIPTION_EX* getDescriptionEx();
};

} // namespace FMOD

#endif
