#ifndef XMODEX_CHANNEL_CHANNEL_EMULATED_HPP
#define XMODEX_CHANNEL_CHANNEL_EMULATED_HPP

#include "xmodex/channel/ChannelReal.hpp"
#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

class ChannelEmulated : public ChannelReal {
    private:
        DSPFilter mDSPHeadMemory;
        DSPI* mDSPHead;

    public:
        ChannelEmulated();

        // virtual ChannelReal methods
        FMOD_RESULT init(int32_t index, SystemI* system, Output* output, DSPI* dspmixtarget);
        FMOD_RESULT close();
        FMOD_RESULT alloc();
        FMOD_RESULT update(int32_t delta);
        FMOD_RESULT stop(bool ignoreenddelay, bool resetstate, bool force);
        FMOD_RESULT isVirtual(bool* isvirtual);
        FMOD_RESULT getDSPHead(DSPI** dsp);
};

} // namespace FMOD

#endif
