#ifndef XMODEX_OUTPUT_OUTPUT_SOFTWARE_HPP
#define XMODEX_OUTPUT_OUTPUT_SOFTWARE_HPP

#include "xmodex/output/Output.hpp"

namespace FMOD {

class ChannelSoftware;

class OutputSoftware : public Output {
    private:
        ChannelSoftware* mChannel;

    public:
        OutputSoftware();

        FMOD_RESULT init(int32_t maxchannels);
        FMOD_RESULT release();
        FMOD_RESULT createSample(FMOD_MODE mode, FMOD_CODEC_WAVEFORMAT* waveformat, Sample** sample);
        int32_t getSampleMaxChannels(FMOD_MODE mode, FMOD_SOUND_FORMAT format);

        static int32_t F_CALLBACK getSampleMaxChannelsCallback(FMOD_OUTPUT_STATE* output, FMOD_MODE mode, FMOD_SOUND_FORMAT format);
};

} // namespace FMOD

#endif
