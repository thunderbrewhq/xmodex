#include "xmodex/LocalCriticalSection.hpp"
#include "xmodex/os/CriticalSection.hpp"

namespace FMOD {

LocalCriticalSection::LocalCriticalSection() {
    this->mEntered = false;
}

LocalCriticalSection::LocalCriticalSection(FMOD_OS_CRITICALSECTION* crit, bool enternow) {
    this->mEntered = false;
    this->mCrit = crit;

    if (enternow) {
        this->enter();
    }
}

LocalCriticalSection::~LocalCriticalSection() {
    if (this->mEntered) {
        this->leave();
    }
}

void LocalCriticalSection::enter() {
    FMOD_OS_CriticalSection_Enter(this->mCrit);
    this->mEntered = true;
}

void LocalCriticalSection::leave() {
    FMOD_OS_CriticalSection_Leave(this->mCrit);
    this->mEntered = false;
}

} // namespace FMOD
