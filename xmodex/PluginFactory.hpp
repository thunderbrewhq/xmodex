#ifndef XMODEX_PLUGIN_FACTORY_HPP
#define XMODEX_PLUGIN_FACTORY_HPP

#include "xmodex/String.hpp"
#include "xmodex/codec/Codec.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/output/Output.hpp"

namespace FMOD {

class SystemI;

class PluginFactory {
    private:
        char mPluginPath[FMOD_STRING_MAXPATHLEN];
        FMOD_CODEC_DESCRIPTION_EX mCodecHead;
        FMOD_DSP_DESCRIPTION_EX mDSPHead;
        FMOD_OUTPUT_DESCRIPTION_EX mOutputHead;
        SystemI* mSystem;

    public:
        PluginFactory();

        FMOD_RESULT createCodec(FMOD_CODEC_DESCRIPTION_EX* codecdesc, Codec** codec);
        FMOD_RESULT createDSP(FMOD_DSP_DESCRIPTION_EX* dspdesc, DSPI** dsp);
        FMOD_RESULT createOutput(FMOD_OUTPUT_DESCRIPTION_EX* outputdesc, Output** output);
        FMOD_RESULT getCodec(int32_t handle, FMOD_CODEC_DESCRIPTION_EX** codecdesc);
        FMOD_RESULT getDSP(int32_t handle, FMOD_DSP_DESCRIPTION_EX** dspdesc);
        FMOD_RESULT getNumCodecs(int32_t* numcodecs);
        FMOD_RESULT getNumDSPs(int32_t* numdsps);
        FMOD_RESULT getNumOutputs(int32_t* numoutputs);
        FMOD_RESULT getOutput(int32_t handle, FMOD_OUTPUT_DESCRIPTION_EX** outputdesc);
        FMOD_RESULT registerCodec(FMOD_CODEC_DESCRIPTION_EX* description, FMOD_PLUGINTYPE* plugintype, int32_t* handle);
        FMOD_RESULT registerDSP(FMOD_DSP_DESCRIPTION_EX* description, FMOD_PLUGINTYPE* plugintype, int32_t* handle);
        FMOD_RESULT registerOutput(FMOD_OUTPUT_DESCRIPTION_EX* description, FMOD_PLUGINTYPE* plugintype, int32_t* handle);
        FMOD_RESULT unloadPlugin(FMOD_PLUGINTYPE plugintype, int32_t handle);
        FMOD_RESULT release();
        FMOD_RESULT setPluginPath(const char* path);
        FMOD_RESULT setSystem(SystemI* system);
};

} // namespace FMOD

#endif
