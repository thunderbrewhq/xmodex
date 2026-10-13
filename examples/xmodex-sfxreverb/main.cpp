#include "fmod.hpp"
#include "fmod_errors.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

void check_result(const char* message, FMOD_RESULT result) {
    if (result != FMOD_OK) {
        printf("%s: FMOD error! (%d) %s\n", message, result, FMOD_ErrorString(result));
        exit(-1);
    } else {
        printf("%s: success\n", message);
    }
}

int main(int argc, char* argv[]) {
    const char* sound_file = "../media/boi.wav";
    if (argc > 1) {
        sound_file = argv[1];
    }

    FMOD_RESULT result;
    FMOD::System* system;
    FMOD::Sound* sound1;
    FMOD::Channel* channel;

    result = FMOD::System_Create(&system);
    check_result("System_Create", result);

    uint32_t version;
    result = system->getVersion(&version);
    check_result("getVersion()", result);

    printf("%x\n", version);

    result = system->setOutput(FMOD_OUTPUTTYPE_AUTODETECT);
    check_result("setOutput(FMOD_OUTPUTTYPE_AUTODETECT)", result);

    result = system->init(32, FMOD_INIT_NORMAL, 0);
    check_result("init(32, FMOD_INIT_NORMAL, 0)", result);

    result = system->createSound(sound_file, FMOD_SOFTWARE, 0, &sound1);
    check_result("createSound()", result);

    result = system->playSound(FMOD_CHANNEL_FREE, sound1, false, &channel);
    check_result("playSound()", result);

    FMOD_REVERB_PROPERTIES reverbprops = FMOD_PRESET_CONCERTHALL;

    result = system->setReverbProperties(&reverbprops);
    check_result("setReverbProperties()", result);

    bool isplaying = true;
    while (isplaying) {
        system->update();

        if (channel) {
            channel->isPlaying(&isplaying);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    result = sound1->release();
    check_result("sound1->release()", result);

    result = system->release();
    check_result("system->release()", result);

    return 0;
}
