#ifndef XMODEX_REVERB_I_HPP
#define XMODEX_REVERB_I_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"

namespace FMOD {

class SystemI;
class DSPI;
class DSPConnection;

enum FMOD_REVERB_MODE {
    FMOD_REVERB_PHYSICAL = 1,
    FMOD_REVERB_VIRTUAL = 2,
};

struct FMOD_REVERB_STDPROPERTIES {
        int32_t Environment;
        float Room;
        float RoomHF;
        float RoomLF;
        float DecayTime;
        float DecayHFRatio;
        float Reflections;
        float ReflectionsDelay;
        float Reverb;
        float ReverbDelay;
        float HFReference;
        float LFReference;
        float RoomRolloffFactor;
        float Diffusion;
        float Density;
};

struct FMOD_REVERB_CHANNELDATA {
        FMOD_REVERB_CHANNELPROPERTIES mChanProps;
        DSPConnection* mDSPConnection;
};

class ReverbI : public LinkedListNode {
        friend class SystemI;

    public:
        SystemI* mSystem;
        DSPI* mDSP;

    private:
        FMOD_REVERB_CHANNELDATA* mChannelData;
        FMOD_REVERB_PROPERTIES mProps;
        void* mUserData;
        float* mPresenceGain;
        float mGain;
        bool mDisableIfNoEnvironment;
        bool m3D;
        bool mActive;
        FMOD_REVERB_MODE mMode;
        FMOD_VECTOR mPosition;
        float mMinRadius;
        float mMaxRadius;
        float mMinMaxDistance;

    public:
        ReverbI();
        ~ReverbI();

        bool is3d();
        FMOD_RESULT setGain(float lingain);
        float getGain();
        FMOD_REVERB_MODE getMode();

        FMOD_RESULT createDSP();
        FMOD_RESULT setChanProperties(int32_t index, const FMOD_REVERB_CHANNELPROPERTIES* props, DSPConnection* connection);
        FMOD_RESULT getChanProperties(int32_t index, FMOD_REVERB_CHANNELPROPERTIES* props, DSPConnection** connection);
        FMOD_RESULT get3DAttributes(FMOD_VECTOR* position, float* mindistance, float* maxdistance);
        FMOD_RESULT setProperties(const FMOD_REVERB_PROPERTIES* prop_source);
        FMOD_RESULT getProperties(FMOD_REVERB_PROPERTIES* properties);
        FMOD_RESULT getActive(bool* active);
        FMOD_RESULT setPresenceGain(int32_t index, float lingain);
        FMOD_RESULT getPresenceGain(int32_t index, float* lingain);
        FMOD_RESULT releaseDSP();
        FMOD_RESULT release(bool freethis);
        FMOD_RESULT init(SystemI* system, bool is_3d, FMOD_REVERB_MODE mode);

        void setDisableIfNoEnvironment(bool dis);
        void calculateDistanceGain(FMOD_VECTOR* p, float* linear_gain, float* linear_coeff);

        static void sumRoomProps(FMOD_REVERB_STDPROPERTIES* accprops, FMOD_REVERB_PROPERTIES* addprops, float factor);
        static void sumProps(FMOD_REVERB_STDPROPERTIES* accprops, FMOD_REVERB_PROPERTIES* addprops, float factor);
        static void factorProps(FMOD_REVERB_PROPERTIES* quotientprops, FMOD_REVERB_STDPROPERTIES* divprops, float factor);
};

} // namespace FMOD

#endif
