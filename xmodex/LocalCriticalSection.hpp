#ifndef XMODEX_LOCAL_CRITICAL_SECTION_HPP
#define XMODEX_LOCAL_CRITICAL_SECTION_HPP

struct FMOD_OS_CRITICALSECTION;

namespace FMOD {

class LocalCriticalSection {
    private:
        FMOD_OS_CRITICALSECTION* mCrit;
        bool mEntered;

    public:
        LocalCriticalSection();
        LocalCriticalSection(FMOD_OS_CRITICALSECTION* crit, bool enternow = false);
        ~LocalCriticalSection();
        void enter();
        void leave();
};

} // namespace FMOD

#endif
