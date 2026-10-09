#ifndef XMODEX_LISTENER_HPP
#define XMODEX_LISTENER_HPP

#include "fmod.hpp"

namespace FMOD {

const int32_t LISTENER_MAX = 4;

class Listener {
    public:
        FMOD_VECTOR mPosition;
        FMOD_VECTOR mLastPosition;
        FMOD_VECTOR mVelocity;
        FMOD_VECTOR mLastVelocity;
        FMOD_VECTOR mUp;
        FMOD_VECTOR mLastUp;
        FMOD_VECTOR mFront;
        FMOD_VECTOR mLastFront;
        FMOD_VECTOR mRight;
        bool mMoved;
        bool mRotated;

    public:
        Listener();
};

} // namespace FMOD

#endif
