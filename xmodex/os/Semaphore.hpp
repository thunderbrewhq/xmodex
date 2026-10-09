#ifndef XMODEX_OS_SEMAPHORE_HPP
#define XMODEX_OS_SEMAPHORE_HPP

#include "fmod.hpp"

struct FMOD_OS_SEMAPHORE;

FMOD_RESULT FMOD_OS_Semaphore_Create(FMOD_OS_SEMAPHORE** sema);

FMOD_RESULT FMOD_OS_Semaphore_Free(FMOD_OS_SEMAPHORE* sema);

FMOD_RESULT FMOD_OS_Semaphore_Signal(FMOD_OS_SEMAPHORE* sema, bool interrupt = false);

FMOD_RESULT FMOD_OS_Semaphore_Wait(FMOD_OS_SEMAPHORE* sema);

#endif
