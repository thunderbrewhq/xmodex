#include "xmodex/channel/ChannelPool.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/channel/ChannelReal.hpp"

namespace FMOD {

ChannelPool::ChannelPool() {
    this->mChannel = nullptr;
    this->mNumChannels = 0;
    this->mChannelsUsed = 0;
    this->mSystem = nullptr;
    this->mOutput = nullptr;
}

FMOD_RESULT ChannelPool::init(SystemI* system, Output* output, int32_t numchannels) {
    if (numchannels < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (numchannels) {
        this->mChannel = reinterpret_cast<ChannelReal**>(FMOD_Memory_Calloc(sizeof(ChannelReal*) * numchannels));
        if (!this->mChannel) {
            return FMOD_ERR_INVALID_PARAM;
        }
    }

    this->mNumChannels = numchannels;
    this->mOutput = output;
    this->mSystem = system;

    return FMOD_OK;
}

FMOD_RESULT ChannelPool::release() {
    if (this->mChannel) {
        for (int32_t i = 0; i < this->mNumChannels; i++) {
            if (this->mChannel[i]) {
                this->mChannel[i]->close();
            }
        }

        FMOD_Memory_Free(this->mChannel);
    }

    FMOD_Memory_Free(this);

    return FMOD_OK;
}

FMOD_RESULT ChannelPool::allocateChannel(ChannelReal** realchannel, int32_t index, int32_t numchannels, int32_t* found_out) {
    FMOD_RESULT result;
    int32_t found = 0;

    if (!realchannel) {
        if (found_out) {
            *found_out = 0;
        }

        return FMOD_ERR_INVALID_PARAM;
    }

    if (index == FMOD_CHANNEL_FREE) {
        for (int32_t i = 0; i < this->mNumChannels; i++) {
            if (!(this->mChannel[i]->mFlags & CHANNELREAL_FLAG_ALLOCATED) &&
                !(this->mChannel[i]->mFlags & (CHANNELREAL_FLAG_IN_USE | CHANNELREAL_FLAG_ENDDELAY | CHANNELREAL_FLAG_RESERVED))) {
                bool playing;
                result = this->mChannel[i]->isPlaying(&playing, true);
                if (result == FMOD_OK && !playing) {
                    this->mChannel[i]->mFlags |= CHANNELREAL_FLAG_ALLOCATED;
                    this->mChannel[i]->mFlags |= CHANNELREAL_FLAG_IN_USE;
                    this->mChannel[i]->mFlags &= ~CHANNELREAL_FLAG_STOPPED;

                    realchannel[found] = this->mChannel[i];
                    found++;

                    if (found == numchannels) {
                        if (found_out) {
                            *found_out = found;
                        }
                        return FMOD_OK;
                    }
                }
            }
        }
    } else if (index >= 0 && index < this->mNumChannels) {
        if (numchannels > 1) {
            return FMOD_ERR_CHANNEL_ALLOC;
        }

        this->mChannel[index]->mFlags |= CHANNELREAL_FLAG_ALLOCATED;
        this->mChannel[index]->mFlags |= CHANNELREAL_FLAG_IN_USE;
        this->mChannel[index]->mFlags &= ~CHANNELREAL_FLAG_STOPPED;

        realchannel[0] = this->mChannel[index];
        return FMOD_OK;
    }

    for (int32_t i = 0; i < found; i++) {
        if (realchannel[i]) {
            realchannel[i]->mFlags &= ~CHANNELREAL_FLAG_ALLOCATED;
            realchannel[i]->mFlags &= ~CHANNELREAL_FLAG_IN_USE;
            realchannel[i]->mFlags |= CHANNELREAL_FLAG_STOPPED;
        }
    }

    if (found_out) {
        *found_out = found;
    }

    return FMOD_ERR_CHANNEL_ALLOC;
}

FMOD_RESULT ChannelPool::getNumChannels(int32_t* numchannels) {
    if (!numchannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numchannels = this->mNumChannels;
    return FMOD_OK;
}

FMOD_RESULT ChannelPool::getChannelsUsed(int32_t* numchannels) {
    if (!numchannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numchannels = this->mChannelsUsed;
    return FMOD_OK;
}

FMOD_RESULT ChannelPool::setChannel(int32_t index, ChannelReal* channel, DSPI* dspmixtarget) {
    if (!channel || index < 0 || index >= this->mNumChannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mChannel[index] = channel;
    this->mChannel[index]->mPool = this;

    return this->mChannel[index]->init(index, this->mSystem, this->mOutput, dspmixtarget);
}

FMOD_RESULT ChannelPool::getChannel(int32_t index, ChannelReal** channel) {
    if (!channel || index < 0 || index >= this->mNumChannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *channel = this->mChannel[index];
    return FMOD_OK;
}

} // namespace FMOD
