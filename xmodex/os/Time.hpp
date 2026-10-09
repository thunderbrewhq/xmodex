#ifndef XMODEX_OS_TIME_HPP
#define XMODEX_OS_TIME_HPP

#include "fmod.hpp"

FMOD_RESULT FMOD_OS_Time_GetMs(uint32_t* ms);

FMOD_RESULT FMOD_OS_Time_GetNs(uint32_t* ns);

FMOD_RESULT FMOD_OS_Time_Sleep(uint32_t ms);

#endif
