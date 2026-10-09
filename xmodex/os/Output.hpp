#ifndef XMODEX_OS_OUTPUT_HPP
#define XMODEX_OS_OUTPUT_HPP

#include "fmod.hpp"

namespace FMOD {

class PluginFactory;

}

FMOD_RESULT FMOD_OS_Output_GetDefault(FMOD_OUTPUTTYPE* outputtype);

FMOD_RESULT FMOD_OS_Output_Register(FMOD::PluginFactory* pluginfactory);

#endif
