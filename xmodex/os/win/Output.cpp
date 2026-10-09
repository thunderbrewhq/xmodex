#include "xmodex/os/Output.hpp"
#include "xmodex/PluginFactory.hpp"
#include "xmodex/output/win/OutputWASAPI.hpp"

FMOD_RESULT FMOD_OS_Output_GetDefault(FMOD_OUTPUTTYPE* outputtype) {
    // OSVERSIONINFO ver;

    if (!outputtype) {
        return FMOD_ERR_INVALID_PARAM;
    }

    // FMOD_memset(&ver, 0, sizeof(ver));
    // ver.dwOSVersionInfoSize = sizeof(ver);
    // GetVersionEx(&ver);
    // *outputtype = ver.dwMajorVersion < 6 ? FMOD_OUTPUTTYPE_DSOUND : FMOD_OUTPUTTYPE_WASAPI;

    *outputtype = FMOD_OUTPUTTYPE_WASAPI;

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Output_Register(FMOD::PluginFactory* pluginfactory) {
    // pluginfactory->registerOutput(FMOD::OutputDSound::getDescriptionEx(), nullptr, nullptr);
    // pluginfactory->registerOutput(FMOD::OutputWinMM::getDescriptionEx(), nullptr, nullptr);
    pluginfactory->registerOutput(FMOD::OutputWASAPI::getDescriptionEx(), nullptr, nullptr);
    // pluginfactory->registerOutput(FMOD::OutputASIO::getDescriptionEx(), nullptr, nullptr);
    // pluginfactory->registerOutput(FMOD::OutputOpenAL::getDescriptionEx(), nullptr, nullptr);

    return FMOD_OK;
}
