#ifndef XMODEX_SPEAKER_LEVELS_POOL_HPP
#define XMODEX_SPEAKER_LEVELS_POOL_HPP

#include "fmod.hpp"

namespace FMOD {

class SystemI;

class LevelsInfo {
    public:
        bool inuse;
        float* levelsmemory;
};

class SpeakerLevelsPool {
        friend class SystemI;

    private:
        LevelsInfo* mLevelsPool;
        SystemI* mSystem;

    public:
        SpeakerLevelsPool();

        FMOD_RESULT alloc(float** levels);
        FMOD_RESULT free(float* levels);
        FMOD_RESULT release();
};

}; // namespace FMOD

#endif
