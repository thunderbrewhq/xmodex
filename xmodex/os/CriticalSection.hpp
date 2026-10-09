#ifndef XMODEX_OS_CRITICAL_SECTION_HPP
#define XMODEX_OS_CRITICAL_SECTION_HPP

#include "fmod.hpp"

struct FMOD_OS_CRITICALSECTION;

FMOD_RESULT FMOD_OS_CriticalSection_Create(FMOD_OS_CRITICALSECTION** crit, bool memorycrit = false);

FMOD_RESULT FMOD_OS_CriticalSection_Free(FMOD_OS_CRITICALSECTION* crit);

FMOD_RESULT FMOD_OS_CriticalSection_Enter(FMOD_OS_CRITICALSECTION* crit);

FMOD_RESULT FMOD_OS_CriticalSection_Leave(FMOD_OS_CRITICALSECTION* crit);

#endif
