#include "xmodex/os/Output.hpp"
#include "fmod.hpp"
#include "xmodex/PluginFactory.hpp"
#include "xmodex/output/bsd/OutputOSS.hpp"

FMOD_RESULT FMOD_OS_Output_GetDefault(FMOD_OUTPUTTYPE* outputtype) {
    if (!outputtype) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *outputtype = FMOD_OUTPUTTYPE_OSS;
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Output_Register(FMOD::PluginFactory* pluginfactory) {
    pluginfactory->registerOutput(FMOD::OutputOSS::getDescriptionEx(), nullptr, nullptr);
    return FMOD_OK;
}
