#ifndef XMODEX_OUTPUT_OUTPUT_EMULATED_HPP
#define XMODEX_OUTPUT_OUTPUT_EMULATED_HPP

#include "xmodex/output/Output.hpp"

namespace FMOD {

class ChannelEmulated;

class OutputEmulated : public Output {
    private:
        ChannelEmulated* mChannel;

    public:
        OutputEmulated();

        FMOD_RESULT release();
        FMOD_RESULT init(int32_t maxchannels);
        FMOD_RESULT update();
};

} // namespace FMOD
#endif
