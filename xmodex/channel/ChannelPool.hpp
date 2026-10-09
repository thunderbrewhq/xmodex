#ifndef XMODEX_CHANNEL_CHANNEL_POOL_HPP
#define XMODEX_CHANNEL_CHANNEL_POOL_HPP

#include "fmod.hpp"

namespace FMOD {

class SystemI;
class Output;
class ChannelReal;
class DSPI;

class ChannelPool {
    public:
        int32_t mNumChannels;
        int32_t mChannelsUsed;
        SystemI* mSystem;
        Output* mOutput;
        ChannelReal** mChannel;

    public:
        ChannelPool();

        FMOD_RESULT init(SystemI* system, Output* output, int32_t numchannels);
        FMOD_RESULT release();
        FMOD_RESULT allocateChannel(ChannelReal** realchannel, int32_t index, int32_t numchannels, int32_t* found_out);
        FMOD_RESULT getNumChannels(int32_t* numchannels);
        FMOD_RESULT getChannelsUsed(int32_t* numchannels);
        FMOD_RESULT setChannel(int32_t index, ChannelReal* channel, DSPI* dspmixtarget);
        FMOD_RESULT getChannel(int32_t index, ChannelReal** channel);
};

} // namespace FMOD

#endif
