#ifndef XMODEX_SOUND_STREAM_HPP
#define XMODEX_SOUND_STREAM_HPP

#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

class Sample;
class ChannelStream;

class Stream : public SoundI {
    public:
        LinkedListNode mStreamNode;
        ChannelStream* mChannel;
        Sample* mSample;
        uint32_t mLastPos;
        int32_t mBlockSize;
        int32_t mLoopCountCurrent;
        int32_t mInitialPosition;

    public:
        Stream();

        FMOD_RESULT fill(uint32_t offset, uint32_t length);
        FMOD_RESULT flush();
        FMOD_RESULT getPosition(uint32_t* position, FMOD_TIMEUNIT postype);
        FMOD_RESULT setPosition(uint32_t position, FMOD_TIMEUNIT postype);

        // virtual SoundI methods
        bool isStream();
        FMOD_RESULT setLoopCount(int32_t loopcount);
};

} // namespace FMOD

#endif
