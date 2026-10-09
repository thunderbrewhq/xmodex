#ifndef XMODEX_SOUND_SAMPLE_HPP
#define XMODEX_SOUND_SAMPLE_HPP

#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

class Sample : public SoundI {
    public:
        int32_t mNumSubSamples;
        Sample* mSubSample[16];
        void* mLockBuffer;
        uint32_t mLockLength;
        uint32_t mLockOffset;
        bool mLockCanRead;

    public:
        Sample();

        // virtual SoundI methods
        FMOD_RESULT release(bool freethis);
        FMOD_RESULT lock(
            uint32_t offset,
            uint32_t length,
            void** ptr1,
            void** ptr2,
            uint32_t* len1,
            uint32_t* len2);
        FMOD_RESULT unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2);
        FMOD_RESULT setDefaults(
            float frequency,
            float volume,
            float pan,
            int32_t priority);
        FMOD_RESULT setVariations(float frequencyvar, float volumevar, float panvar);
        FMOD_RESULT set3DMinMaxDistance(float min, float max);
        FMOD_RESULT set3DConeSettings(
            float insideconeangle,
            float outsideconeangle,
            float outsidevolume);
        FMOD_RESULT setMode(FMOD_MODE mode);
        FMOD_RESULT setLoopCount(int32_t loopcount);
        FMOD_RESULT setLoopPoints(
            uint32_t loopstart,
            FMOD_TIMEUNIT loopstarttype,
            uint32_t loopend,
            FMOD_TIMEUNIT loopendtype);

        // virtual Sample methods
        virtual FMOD_RESULT lockInternal(uint32_t offset,
                                         uint32_t length,
                                         void** ptr1,
                                         void** ptr2,
                                         uint32_t* len1,
                                         uint32_t* len2);
        virtual FMOD_RESULT unlockInternal(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2);
        virtual FMOD_RESULT setBufferData(void* data);
};

} // namespace FMOD

#endif
