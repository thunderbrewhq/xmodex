#include "xmodex/os/Output.hpp"
#include "fmod.hpp"
#include "xmodex/PluginFactory.hpp"
#include "xmodex/output/bsd/OutputOSS.hpp"
#include "xmodex/output/linux/OutputALSA.hpp"

FMOD_RESULT FMOD_OS_Output_GetDefault(FMOD_OUTPUTTYPE* outputtype) {
    if (!outputtype) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *outputtype = FMOD_OUTPUTTYPE_ALSA;
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Output_Register(FMOD::PluginFactory* pluginfactory) {
    pluginfactory->registerOutput(FMOD::OutputOSS::getDescriptionEx(), nullptr, nullptr);
    pluginfactory->registerOutput(FMOD::OutputALSA::getDescriptionEx(), nullptr, nullptr);
    // pluginfactory->registerOutput(FMOD::OutputESD::getDescriptionEx(), nullptr, nullptr);
    return FMOD_OK;
}
