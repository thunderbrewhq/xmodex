#include "fmod.hpp"
#include "xmodex/channel/ChannelGroupI.hpp"

namespace FMOD {

ChannelGroup::ChannelGroup() {
}

FMOD_RESULT F_API ChannelGroup::release() {
    FMOD_RESULT result;
    ChannelGroupI* channelgroupi;

    result = ChannelGroupI::validate(this, &channelgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channelgroupi->release();
    }
}

FMOD_RESULT F_API ChannelGroup::setVolume(float volume) {
    FMOD_RESULT result;
    ChannelGroupI* channelgroupi;

    result = ChannelGroupI::validate(this, &channelgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channelgroupi->setVolume(volume);
    }
}

FMOD_RESULT F_API ChannelGroup::getDSPHead(DSP** dsp) {
    FMOD_RESULT result;
    ChannelGroupI* channelgroupi;

    result = ChannelGroupI::validate(this, &channelgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channelgroupi->getDSPHead(reinterpret_cast<DSPI**>(dsp));
    }
}

FMOD_RESULT F_API ChannelGroup::getNumChannels(int32_t* numchannels) {
    FMOD_RESULT result;
    ChannelGroupI* channelgroupi;

    result = ChannelGroupI::validate(this, &channelgroupi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return channelgroupi->getNumChannels(numchannels);
    }
}

} // namespace FMOD
