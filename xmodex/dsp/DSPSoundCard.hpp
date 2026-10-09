#ifndef XMODEX_DSP_DSP_SOUND_CARD_HPP
#define XMODEX_DSP_DSP_SOUND_CARD_HPP

#include "xmodex/dsp/DSPFilter.hpp"

namespace FMOD {

class DSPSoundCard : public DSPFilter {
    private:
        float* mConvertBuffer;
        float* mConvertBufferMemory;

    public:
        DSPSoundCard();
        FMOD_RESULT alloc(FMOD_DSP_DESCRIPTION_EX* description);
        FMOD_RESULT execute(void* inbuffer, void** outbuffer, uint32_t* length, int32_t inchannels, int32_t* outchannels, FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT release(bool freethis);
};

} // namespace FMOD

#endif
