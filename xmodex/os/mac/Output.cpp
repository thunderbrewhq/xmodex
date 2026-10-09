#include "xmodex/os/Output.hpp"
#include "xmodex/PluginFactory.hpp"
#include "xmodex/output/mac/OutputCoreAudio.hpp"

FMOD_RESULT FMOD_OS_Output_GetDefault(FMOD_OUTPUTTYPE* outputtype) {
    if (!outputtype) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *outputtype = FMOD_OUTPUTTYPE_COREAUDIO;
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Output_Register(FMOD::PluginFactory* pluginfactory) {
    // pluginfactory->registerOutput(FMOD::OutputSoundManager::getDescriptionEx(), 0, 0);
    pluginfactory->registerOutput(FMOD::OutputCoreAudio::getDescriptionEx(), nullptr, nullptr);
    return FMOD_OK;
}
