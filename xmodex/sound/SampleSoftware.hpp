#ifndef XMODEX_SOUND_SAMPLE_SOFTWARE_HPP
#define XMODEX_SOUND_SAMPLE_SOFTWARE_HPP

#include "xmodex/sound/Sample.hpp"

namespace FMOD {

class SampleSoftware : public Sample {
        friend class OutputSoftware;
        friend class ChannelSoftware;
        friend class DSPWaveTable;
        friend class DSPCodec;

    private:
        void* mBuffer;
        void* mBufferMemory;
        char* mLoopPointDataEnd;
        char mLoopPointDataEndMemory[8];
        bool mDataEndCopied;

    public:
        SampleSoftware();

        FMOD_RESULT release(bool freethis);

        FMOD_RESULT restoreLoopPointData();

        // virtual Sample methods
        FMOD_RESULT lockInternal(
            uint32_t offset,
            uint32_t length,
            void** ptr1,
            void** ptr2,
            uint32_t* len1,
            uint32_t* len2);
        FMOD_RESULT unlockInternal(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2);
        FMOD_RESULT setBufferData(void* data);

        // virtual SoundI methods
        FMOD_RESULT setLoopPointData();
        FMOD_RESULT setMode(FMOD_MODE mode);
};

} // namespace FMOD

#endif
