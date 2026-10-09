#ifndef XMODEX_CHANNEL_CHANNEL_REAL_MANUAL_3D_HPP
#define XMODEX_CHANNEL_CHANNEL_REAL_MANUAL_3D_HPP

#include "xmodex/channel/ChannelReal.hpp"

namespace FMOD {

class ChannelRealManual3D : public ChannelReal {
        friend class ChannelSoftware;

    private:
        float mAngleToListener;

    public:
        ChannelRealManual3D();

        FMOD_RESULT set2DFreqVolumePanFor3D();
};

} // namespace FMOD

#endif
