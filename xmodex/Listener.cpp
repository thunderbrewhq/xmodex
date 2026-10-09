#include "xmodex/Listener.hpp"

namespace FMOD {

Listener::Listener() {
    this->mPosition.x = 0.0f;
    this->mPosition.y = 0.0f;
    this->mPosition.z = 0.0f;
    this->mLastPosition.x = 0.0f;
    this->mLastPosition.y = 0.0f;
    this->mLastPosition.z = 0.0f;
    this->mVelocity.x = 0.0f;
    this->mVelocity.y = 0.0f;
    this->mVelocity.z = 0.0f;
    this->mUp.x = 0.0f;
    this->mUp.y = 1.0f;
    this->mFront.z = 1.0f;
    this->mRight.x = 1.0f;
    this->mUp.z = 0.0f;
    this->mFront.x = 0.0f;
    this->mFront.y = 0.0f;
    this->mRight.y = 0.0f;
    this->mRight.z = 0.0f;
}

} // namespace FMOD
