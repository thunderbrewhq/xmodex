#include "xmodex/output/OutputEmulated.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/channel/ChannelEmulated.hpp"
#include "xmodex/channel/ChannelPool.hpp"

namespace FMOD {

OutputEmulated::OutputEmulated() {
    memset(&this->mDescription, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));
    this->mChannel = nullptr;
}

FMOD_RESULT OutputEmulated::release() {
    if (this->mChannelPool) {
        this->mChannelPool->release();
        this->mChannelPool = nullptr;
    }

    if (this->mChannel) {
        FMOD_Memory_Free(this->mChannel);
        this->mChannel = nullptr;
    }

    return Output::release();
}

FMOD_RESULT OutputEmulated::init(int32_t maxchannels) {
    FMOD_RESULT result;

    if (!this->mSystem) {
        return FMOD_ERR_UNINITIALIZED;
    }

    if (maxchannels) {
        this->mChannelPool3D = FMOD_Object_Alloc(ChannelPool);
        this->mChannelPool = this->mChannelPool3D;
        if (!this->mChannelPool) {
            return FMOD_ERR_MEMORY;
        }

        result = this->mChannelPool->init(this->mSystem, this, maxchannels);
        if (result != FMOD_OK) {
            return result;
        }

        this->mChannel = static_cast<ChannelEmulated*>(FMOD_Memory_Calloc(sizeof(ChannelEmulated) * maxchannels));
        if (!this->mChannel) {
            return FMOD_ERR_MEMORY;
        }

        for (int32_t i = 0; i < maxchannels; i++) {
            new (&this->mChannel[i]) ChannelEmulated;
            this->mChannelPool->setChannel(i, &this->mChannel[i], nullptr);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT OutputEmulated::update() {
    return FMOD_OK;
}

} // namespace FMOD
