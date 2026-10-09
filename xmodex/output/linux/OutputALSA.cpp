#include "xmodex/output/linux/OutputALSA.hpp"
#include "fmod.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/output/Output.hpp"
#include "xmodex/output/linux/ALSA.hpp"
#include "xmodex/sound/SoundI.hpp"
#include <cerrno>
#include <cstdio>
#include <dlfcn.h>

snd_pcm_open_t so_snd_pcm_open;
snd_pcm_hw_params_sizeof_t so_snd_pcm_hw_params_sizeof;
snd_pcm_hw_params_any_t so_snd_pcm_hw_params_any;
snd_pcm_hw_params_set_access_t so_snd_pcm_hw_params_set_access;
snd_pcm_hw_params_set_format_t so_snd_pcm_hw_params_set_format;
snd_pcm_hw_params_set_rate_t so_snd_pcm_hw_params_set_rate;
snd_pcm_hw_params_set_rate_near_t so_snd_pcm_hw_params_set_rate_near;
snd_pcm_hw_params_set_channels_t so_snd_pcm_hw_params_set_channels;
snd_pcm_hw_params_set_periods_t so_snd_pcm_hw_params_set_periods;
snd_pcm_hw_params_set_periods_near_t so_snd_pcm_hw_params_set_periods_near;
snd_pcm_hw_params_set_period_time_near_t so_snd_pcm_hw_params_set_period_time_near;
snd_pcm_hw_params_set_buffer_size_t so_snd_pcm_hw_params_set_buffer_size;
snd_pcm_hw_params_set_buffer_size_near_t so_snd_pcm_hw_params_set_buffer_size_near;
snd_pcm_hw_params_set_buffer_time_near_t so_snd_pcm_hw_params_set_buffer_time_near;
snd_pcm_hw_params_get_buffer_size_t so_snd_pcm_hw_params_get_buffer_size;
snd_pcm_hw_params_get_buffer_size_min_t so_snd_pcm_hw_params_get_buffer_size_min;
snd_pcm_hw_params_get_buffer_size_max_t so_snd_pcm_hw_params_get_buffer_size_max;
snd_pcm_hw_params_get_buffer_time_min_t so_snd_pcm_hw_params_get_buffer_time_min;
snd_pcm_hw_params_get_buffer_time_max_t so_snd_pcm_hw_params_get_buffer_time_max;
snd_pcm_hw_params_get_period_time_t so_snd_pcm_hw_params_get_period_time;
snd_pcm_hw_params_get_period_size_t so_snd_pcm_hw_params_get_period_size;
snd_pcm_hw_params_t_fn so_snd_pcm_hw_params;
snd_pcm_writei_t so_snd_pcm_writei;
snd_pcm_readi_t so_snd_pcm_readi;
snd_pcm_prepare_t so_snd_pcm_prepare;
snd_pcm_close_t so_snd_pcm_close;
snd_pcm_drop_t so_snd_pcm_drop;
snd_card_next_t so_snd_card_next;
snd_card_get_name_t so_snd_card_get_name;
snd_pcm_nonblock_t so_snd_pcm_nonblock;
snd_strerror_t so_snd_strerror;
snd_pcm_bytes_to_frames_t so_snd_pcm_bytes_to_frames;
snd_pcm_frames_to_bytes_t so_snd_pcm_frames_to_bytes;

namespace FMOD {

FMOD_OUTPUT_DESCRIPTION_EX alsaoutput;

FMOD_RESULT OutputALSA::enumerate() {
    FMOD_RESULT result;
    char name[128];

    if (this->mEnumerated) {
        return FMOD_OK;
    }

    Plugin::init();

    this->mNumDrivers = 0;

    result = this->registerLib();
    if (result != FMOD_OK) {
        return result;
    }

    this->mMaxDrivers = 32;
    this->mDriverNames = static_cast<char**>(FMOD_Memory_Alloc(32 * sizeof(char*)));
    if (!this->mDriverNames) {
        return FMOD_ERR_MEMORY;
    }

    this->mDriverNames[this->mNumDrivers] = static_cast<char*>(FMOD_Memory_Calloc(10));
    if (!this->mDriverNames[0]) {
        return FMOD_ERR_MEMORY;
    }

    strcpy(this->mDriverNames[0], "default");
    this->mNumDrivers++;

    result = this->getALSAdrivers("/usr/share/alsa/alsa.conf");
    if (result != FMOD_OK) {
        this->getALSAdrivers("/etc/alsa/alsa.conf");
    }

    this->getALSAdrivers("/etc/asound.conf");

    sprintf(name, "%s/.asoundrc", getenv("HOME"));

    this->getALSAdrivers(name);

    this->mEnumerated = true;
    return FMOD_OK;
}

FMOD_RESULT OutputALSA::getALSAdrivers(const char* name) {
    auto fp = fopen(name, "r");
    if (!fp) {
        return FMOD_ERR_FILE_NOTFOUND;
    }

    char driver[256];
    char ch;

    do {
        memset(driver, 0, 256);

        for (int32_t i = 0; i < 256; i++) {
            ch = static_cast<char>(fgetc(fp));
            if (ch == '\n') {
                break;
            }
        }

        if (!FMOD_strncmp("pcm.", driver, 4)) {
            uint32_t i = 0;
            uint32_t len;
            do {
                len = i;
                i = len + 1;
            } while (driver[len + 4] != ' ');

            if (FMOD_strncmp("default", driver + 4, 7)) {
                this->mDriverNames[this->mNumDrivers] = static_cast<char*>(FMOD_Memory_Calloc(len + 2));
                if (!this->mDriverNames[this->mNumDrivers]) {
                    return FMOD_ERR_MEMORY;
                }

                FMOD_strncpy(this->mDriverNames[this->mNumDrivers], driver + 4, len);
                this->mNumDrivers++;

                if (this->mNumDrivers >= this->mMaxDrivers) {
                    this->mMaxDrivers *= 2;
                    this->mDriverNames = static_cast<char**>(FMOD_Memory_ReAlloc(this->mDriverNames, this->mMaxDrivers * sizeof(char*)));
                    if (!this->mDriverNames) {
                        return FMOD_ERR_MEMORY;
                    }
                }
            }
        }
    } while (ch != -1);

    fclose(fp);

    return FMOD_OK;
}

FMOD_RESULT OutputALSA::registerLib() {
    if (this->mLibrary) {
        return FMOD_OK;
    }

    this->mLibrary = dlopen("libasound.so", RTLD_GLOBAL | RTLD_LAZY);
    if (!this->mLibrary) {
        this->mLibrary = dlopen("libasound.so.2", RTLD_GLOBAL | RTLD_LAZY);
        if (!this->mLibrary) {
            return FMOD_ERR_OUTPUT_INIT;
        }
    }

    so_snd_pcm_open = reinterpret_cast<snd_pcm_open_t>(dlsym(this->mLibrary, "snd_pcm_open"));
    if (!so_snd_pcm_open) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_sizeof = reinterpret_cast<snd_pcm_hw_params_sizeof_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_sizeof"));
    if (!so_snd_pcm_hw_params_sizeof) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_any = reinterpret_cast<snd_pcm_hw_params_any_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_any"));
    if (!so_snd_pcm_hw_params_any) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_access = reinterpret_cast<snd_pcm_hw_params_set_access_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_access"));
    if (!so_snd_pcm_hw_params_set_access) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_format = reinterpret_cast<snd_pcm_hw_params_set_format_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_format"));
    if (!so_snd_pcm_hw_params_set_format) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_rate = reinterpret_cast<snd_pcm_hw_params_set_rate_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_rate"));
    if (!so_snd_pcm_hw_params_set_rate) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_rate_near = reinterpret_cast<snd_pcm_hw_params_set_rate_near_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_rate_near"));
    if (!so_snd_pcm_hw_params_set_rate_near) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_channels = reinterpret_cast<snd_pcm_hw_params_set_channels_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_channels"));
    if (!so_snd_pcm_hw_params_set_channels) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_periods = reinterpret_cast<snd_pcm_hw_params_set_periods_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_periods"));
    if (!so_snd_pcm_hw_params_set_periods) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_periods_near = reinterpret_cast<snd_pcm_hw_params_set_periods_near_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_periods_near"));
    if (!so_snd_pcm_hw_params_set_periods_near) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_period_time_near = reinterpret_cast<snd_pcm_hw_params_set_period_time_near_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_period_time_near"));
    if (!so_snd_pcm_hw_params_set_period_time_near) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_buffer_size = reinterpret_cast<snd_pcm_hw_params_set_buffer_size_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_buffer_size"));
    if (!so_snd_pcm_hw_params_set_buffer_size) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_buffer_size_near = reinterpret_cast<snd_pcm_hw_params_set_buffer_size_near_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_buffer_size_near"));
    if (!so_snd_pcm_hw_params_set_buffer_size_near) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_set_buffer_time_near = reinterpret_cast<snd_pcm_hw_params_set_buffer_time_near_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_set_buffer_time_near"));
    if (!so_snd_pcm_hw_params_set_buffer_time_near) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_get_buffer_size = reinterpret_cast<snd_pcm_hw_params_get_buffer_size_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_get_buffer_size"));
    if (!so_snd_pcm_hw_params_get_buffer_size) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_get_buffer_size_min = reinterpret_cast<snd_pcm_hw_params_get_buffer_size_min_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_get_buffer_size_min"));
    if (!so_snd_pcm_hw_params_get_buffer_size_min) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_get_buffer_size_max = reinterpret_cast<snd_pcm_hw_params_get_buffer_size_max_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_get_buffer_size_max"));
    if (!so_snd_pcm_hw_params_get_buffer_size_max) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_get_buffer_time_min = reinterpret_cast<snd_pcm_hw_params_get_buffer_time_min_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_get_buffer_time_min"));
    if (!so_snd_pcm_hw_params_get_buffer_time_min) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_get_buffer_time_max = reinterpret_cast<snd_pcm_hw_params_get_buffer_time_max_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_get_buffer_time_max"));
    if (!so_snd_pcm_hw_params_get_buffer_time_max) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_get_period_time = reinterpret_cast<snd_pcm_hw_params_get_period_time_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_get_period_time"));
    if (!so_snd_pcm_hw_params_get_period_time) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params_get_period_size = reinterpret_cast<snd_pcm_hw_params_get_period_size_t>(dlsym(this->mLibrary, "snd_pcm_hw_params_get_period_size"));
    if (!so_snd_pcm_hw_params_get_period_size) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_hw_params = reinterpret_cast<snd_pcm_hw_params_t_fn>(dlsym(this->mLibrary, "snd_pcm_hw_params"));
    if (!so_snd_pcm_hw_params) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_writei = reinterpret_cast<snd_pcm_writei_t>(dlsym(this->mLibrary, "snd_pcm_writei"));
    if (!so_snd_pcm_writei) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_readi = reinterpret_cast<snd_pcm_readi_t>(dlsym(this->mLibrary, "snd_pcm_readi"));
    if (!so_snd_pcm_readi) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_prepare = reinterpret_cast<snd_pcm_prepare_t>(dlsym(this->mLibrary, "snd_pcm_prepare"));
    if (!so_snd_pcm_prepare) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_close = reinterpret_cast<snd_pcm_close_t>(dlsym(this->mLibrary, "snd_pcm_close"));
    if (!so_snd_pcm_close) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_drop = reinterpret_cast<snd_pcm_drop_t>(dlsym(this->mLibrary, "snd_pcm_drop"));
    if (!so_snd_pcm_drop) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_card_next = reinterpret_cast<snd_card_next_t>(dlsym(this->mLibrary, "snd_card_next"));
    if (!so_snd_card_next) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_card_get_name = reinterpret_cast<snd_card_get_name_t>(dlsym(this->mLibrary, "snd_card_get_name"));
    if (!so_snd_card_get_name) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_nonblock = reinterpret_cast<snd_pcm_nonblock_t>(dlsym(this->mLibrary, "snd_pcm_nonblock"));
    if (!so_snd_pcm_nonblock) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_strerror = reinterpret_cast<snd_strerror_t>(dlsym(this->mLibrary, "snd_strerror"));
    if (!so_snd_strerror) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_bytes_to_frames = reinterpret_cast<snd_pcm_bytes_to_frames_t>(dlsym(this->mLibrary, "snd_pcm_bytes_to_frames"));
    if (!so_snd_pcm_bytes_to_frames) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_frames_to_bytes = reinterpret_cast<snd_pcm_frames_to_bytes_t>(dlsym(this->mLibrary, "snd_pcm_frames_to_bytes"));
    if (!so_snd_pcm_frames_to_bytes) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputALSA::updateMixer() {
    uint32_t blocksize;
    uint32_t bytes;
    FMOD_SPEAKERMODE speakermode;

    this->mSystem->getDSPBufferSize(&blocksize, nullptr);
    this->mSystem->getSpeakerMode(&speakermode);

    SoundI::getBytesFromSamples(blocksize, &bytes, this->mNumOutputChannels, this->mOutputFormat);

    this->mSystem->mDSPTimeStamp.stampIn();

    Output::mix(this->mOutputBuffer, blocksize);

    if (speakermode == FMOD_SPEAKERMODE_5POINT1 || speakermode == FMOD_SPEAKERMODE_7POINT1) {
        if (this->mOutputFormat == FMOD_SOUND_FORMAT_PCM8) {
            auto buffer = static_cast<int8_t*>(this->mOutputBuffer);

            for (int32_t i = 0; i < blocksize; i++) {
                auto offset = i * this->mNumOutputChannels;
                auto c2 = buffer[offset + 2];
                auto c3 = buffer[offset + 3];
                auto c4 = buffer[offset + 4];
                auto c5 = buffer[offset + 5];

                buffer[offset + 2] = c4;
                buffer[offset + 3] = c5;
                buffer[offset + 4] = c2;
                buffer[offset + 5] = c3;
            }
        } else if (this->mOutputFormat == FMOD_SOUND_FORMAT_PCM16) {
            auto buffer = static_cast<int16_t*>(this->mOutputBuffer);

            for (int32_t i = 0; i < blocksize; i++) {
                auto offset = i * this->mNumOutputChannels;

                auto c2 = buffer[offset + 2];
                auto c3 = buffer[offset + 3];
                auto c4 = buffer[offset + 4];
                auto c5 = buffer[offset + 5];

                buffer[offset + 2] = c4;
                buffer[offset + 3] = c5;
                buffer[offset + 4] = c2;
                buffer[offset + 5] = c3;
            }
        }
    }

    auto frames = so_snd_pcm_bytes_to_frames(this->mHandle, bytes);
    if (so_snd_pcm_writei(this->mHandle, this->mOutputBuffer, frames) == -EPIPE) {
        so_snd_pcm_prepare(this->mHandle);
    }

    this->mSystem->mDSPTimeStamp.stampOut(95);

    return FMOD_OK;
}

FMOD_RESULT OutputALSA::updateRecord() {
    if (this->mRecording) {
        auto frames = so_snd_pcm_bytes_to_frames(
            this->mRecordHandle,
            this->mRecordBlockLenBytes);

        auto result = so_snd_pcm_readi(
            this->mRecordHandle,
            static_cast<char*>(this->mRecordBuffer) + (this->mRecordCurrentBlock * this->mRecordBlockLenBytes),
            frames);

        if (result == -EPIPE) {
            so_snd_pcm_prepare(this->mRecordHandle);
        } else {
            this->mRecordCurrentBlock++;
            if (this->mRecordCurrentBlock >= RECORD_MAXBLOCKS) {
                this->mRecordCurrentBlock = 0;
            }

            this->mRecordPosition += result;
            if (this->mRecordPosition > this->mRecordBufferLength) {
                this->mRecordPosition = 0;
            }
        }
    }

    return FMOD_OK;
}

void F_CALLBACK OutputALSA::mixThreadCallback(void* data) {
    static_cast<OutputALSA*>(data)->updateMixer();
}

void F_CALLBACK OutputALSA::recordThreadCallback(void* data) {
    static_cast<OutputALSA*>(data)->updateRecord();
}

FMOD_RESULT OutputALSA::getNumDrivers(int32_t* numdrivers) {
    if (!numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mEnumerated) {
        FMOD_RESULT result;
        result = this->enumerate();
        if (result != FMOD_OK) {
            return result;
        }
    }

    *numdrivers = this->mNumDrivers;
    return FMOD_OK;
}

FMOD_RESULT OutputALSA::getDriverName(
    int32_t id,
    char* name,
    int32_t namelen) {

    if (!name || !namelen) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mEnumerated) {
        FMOD_RESULT result;
        result = this->enumerate();
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (id < 0 || id >= this->mNumDrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(name, this->mDriverNames[id], namelen - 1);
    return FMOD_OK;
}

FMOD_RESULT OutputALSA::init(
    int32_t selecteddriver,
    FMOD_INITFLAGS flags,
    int32_t* outputrate,
    int32_t outputchannels,
    FMOD_SOUND_FORMAT* outputformat,
    int32_t dspbufferlength,
    int32_t dspnumbuffers,
    void* extradriverdata) {

    FMOD_RESULT result;
    char name[128];

    result = this->enumerate();

    if (result != FMOD_OK) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (*outputformat == FMOD_SOUND_FORMAT_PCM8 || *outputformat == FMOD_SOUND_FORMAT_PCM16) {
        *outputformat = FMOD_SOUND_FORMAT_PCM16;
    }

    this->mNumOutputChannels = outputchannels;
    this->mOutputFormat = *outputformat;
    this->mOutputRate = *outputrate;

    if (!this->mNumDrivers) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (selecteddriver < 0) {
        const char* device = "surround51";
        if (outputchannels != 6) {
            if (outputchannels < 7) {
                device = this->mDriverNames[0];
            } else {
                device = "surround71";
            }
        }
        strcpy(name, device);
    } else {
        if (extradriverdata) {
            sprintf(name, "%s:%s", this->mDriverNames[selecteddriver], static_cast<char*>(extradriverdata));
        } else {
            strcpy(name, this->mDriverNames[selecteddriver]);
        }
    }

    if (so_snd_pcm_open(&this->mHandle, name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    so_snd_pcm_nonblock(this->mHandle, 0);

    return FMOD_OK;
}

FMOD_RESULT OutputALSA::close() {
    if (this->mHandle) {
        so_snd_pcm_close(this->mHandle);
    }

    if (this->mLibrary) {
        dlclose(this->mLibrary);
    }

    while (this->mNumDrivers) {
        this->mNumDrivers--;
        FMOD_Memory_Free(this->mDriverNames[this->mNumDrivers]);
        this->mDriverNames[this->mNumDrivers] = nullptr;
    }

    if (this->mDriverNames) {
        FMOD_Memory_Free(this->mDriverNames);
        this->mDriverNames = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputALSA::start() {
    FMOD_RESULT result;
    uint32_t bufferlength;
    uint32_t bufferbytes;

    result = this->mSystem->getDSPBufferSize(&bufferlength, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = SoundI::getBytesFromSamples(
        bufferlength,
        &bufferbytes,
        this->mNumOutputChannels,
        this->mOutputFormat);
    if (result != FMOD_OK) {
        return result;
    }

    this->mOutputBuffer = FMOD_Memory_Alloc(bufferbytes);
    if (!this->mOutputBuffer) {
        return FMOD_ERR_MEMORY;
    }

    auto params = static_cast<snd_pcm_hw_params_t*>(alloca(so_snd_pcm_hw_params_sizeof() + 15));
    memset(params, 0, so_snd_pcm_hw_params_sizeof());

    if (so_snd_pcm_hw_params_any(this->mHandle, params) < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (so_snd_pcm_hw_params_set_access(this->mHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (so_snd_pcm_hw_params_set_format(this->mHandle, params, this->mOutputFormat == FMOD_SOUND_FORMAT_PCM8 ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_S16) < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (so_snd_pcm_hw_params_set_rate(this->mHandle, params, this->mOutputRate, 0) < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (so_snd_pcm_hw_params_set_channels(this->mHandle, params, this->mNumOutputChannels) < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (so_snd_pcm_hw_params_set_periods(this->mHandle, params, 2, 0) >= 0) {
        so_snd_pcm_hw_params_set_buffer_size_near(this->mHandle, params, &bufferbytes);
    }

    if (so_snd_pcm_hw_params(this->mHandle, params) < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    return this->mMixerThread.initThread(
        "ALSA Mixer",
        mixThreadCallback,
        this,
        Thread::PRIORITY_HIGH,
        nullptr,
        0,
        false,
        0);
}

FMOD_RESULT OutputALSA::stop() {
    this->mMixerThread.closeThread();

    if (this->mOutputBuffer) {
        FMOD_Memory_Free(this->mOutputBuffer);
        this->mOutputBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputALSA::getHandle(void** handle) {
    *handle = this->mHandle;
    return FMOD_OK;
}

FMOD_RESULT OutputALSA::recordGetNumDrivers(int32_t* numdrivers) {
    return this->getNumDrivers(numdrivers);
}

FMOD_RESULT OutputALSA::recordGetDriverInfo(
    int32_t id,
    char* name,
    int32_t namelen) {
    return this->getDriverName(id, name, namelen);
}

FMOD_RESULT OutputALSA::recordStart(
    int32_t id,
    FMOD_SOUND* sound,
    int32_t loop) {
    uint32_t period_size;
    uint32_t buffer_size;
    uint32_t period_time = 10000;
    uint32_t buffer_time = 20000;

    // ...

    if (!sound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto soundi = reinterpret_cast<SoundI*>(sound);
    auto format = soundi->mFormat;
    auto frequency = static_cast<uint32_t>(soundi->mDefaultFrequency);

    this->mRecordPosition = 0;
    this->mRecordCurrentBlock = 0;
    this->mRecordFormat = format;
    this->mRecordChannels = soundi->mChannels;

    if (id < 0) {
        id = 0;
    }

    if (so_snd_pcm_open(&this->mRecordHandle, this->mDriverNames[id], SND_PCM_STREAM_CAPTURE, 0) < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto params = static_cast<snd_pcm_hw_params_t*>(alloca(so_snd_pcm_hw_params_sizeof() + 15));
    memset(params, 0, so_snd_pcm_hw_params_sizeof());

    if (so_snd_pcm_hw_params_any(this->mRecordHandle, params) < 0) {
        return FMOD_ERR_RECORD;
    }

    if (so_snd_pcm_hw_params_set_access(this->mRecordHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
        return FMOD_ERR_RECORD;
    }

    if (so_snd_pcm_hw_params_set_format(this->mRecordHandle, params, format == FMOD_SOUND_FORMAT_PCM8 ? SND_PCM_FORMAT_S8 : SND_PCM_FORMAT_S16) < 0) {
        return FMOD_ERR_RECORD;
    }

    if (so_snd_pcm_hw_params_set_rate_near(this->mRecordHandle, params, &frequency, nullptr) < 0) {
        return FMOD_ERR_RECORD;
    }

    if (so_snd_pcm_hw_params_set_channels(this->mRecordHandle, params, static_cast<uint32_t>(this->mRecordChannels)) < 0) {
        return FMOD_ERR_RECORD;
    }

    so_snd_pcm_hw_params_set_period_time_near(this->mRecordHandle, params, &period_time, nullptr);
    so_snd_pcm_hw_params_get_period_time(params, &period_time, nullptr);
    so_snd_pcm_hw_params_get_period_size(params, &period_size, nullptr);

    if (static_cast<int32_t>(period_size) <= 0) {
        return FMOD_ERR_RECORD;
    }

    so_snd_pcm_hw_params_set_buffer_time_near(this->mRecordHandle, params, &buffer_time, nullptr);
    so_snd_pcm_hw_params_get_buffer_size(params, &buffer_size);

    SoundI::getBytesFromSamples(buffer_size, &this->mRecordBlockLenBytes, this->mRecordChannels, this->mRecordFormat);
    this->mRecordBufferBytes = this->mRecordBlockLenBytes * RECORD_MAXBLOCKS;
    SoundI::getBytesFromSamples(this->mRecordBufferBytes, &this->mRecordBufferLength, this->mRecordChannels, this->mRecordFormat);

    if (so_snd_pcm_hw_params(this->mRecordHandle, params) < 0) {
        return FMOD_ERR_RECORD;
    }

    this->mRecordBuffer = FMOD_Memory_Alloc(this->mRecordBufferBytes);
    if (!this->mRecordBuffer) {
        return FMOD_ERR_MEMORY;
    }

    so_snd_pcm_prepare(this->mRecordHandle);

    return this->mRecordThread.initThread(
        "ALSA Record",
        recordThreadCallback,
        this,
        Thread::PRIORITY_NORMAL,
        nullptr,
        0,
        false,
        0);
}

FMOD_RESULT OutputALSA::recordStop() {
    this->mRecordThread.closeThread();

    if (this->mRecordBuffer) {
        FMOD_Memory_Free(this->mRecordBuffer);
        this->mRecordBuffer = nullptr;
    }

    if (this->mRecordHandle) {
        so_snd_pcm_close(this->mRecordHandle);
        this->mRecordHandle = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputALSA::recordGetPosition(uint32_t* pcm) {
    *pcm = this->mRecordPosition;
    return FMOD_OK;
}

FMOD_RESULT OutputALSA::recordLock(
    uint32_t offset,
    uint32_t length,
    void** ptr1,
    void** ptr2,
    uint32_t* len1,
    uint32_t* len2) {

    uint32_t lenbytes = this->mRecordBlockLenBytes * RECORD_MAXBLOCKS;

    if (length > lenbytes) {
        length = lenbytes;
    }

    if (offset >= lenbytes) {
        *ptr1 = nullptr;
        *ptr2 = nullptr;
        *len1 = 0;
        *len2 = 0;
        return FMOD_ERR_INVALID_PARAM;
    }

    if (offset + length <= lenbytes) {
        *ptr1 = static_cast<uint8_t*>(this->mRecordBuffer) + offset;
        *len1 = length;
        *ptr2 = nullptr;
        *len2 = 0;
    } else {
        *ptr1 = static_cast<uint8_t*>(this->mRecordBuffer) + offset;
        *len1 = lenbytes - offset;
        *ptr2 = this->mRecordBuffer;
        *len2 = length - (lenbytes - offset);
    }

    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK OutputALSA::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputALSA::getDriverNameCallback(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    char* name,
    int32_t namelen) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputALSA::initCallback(
    FMOD_OUTPUT_STATE* output,
    int32_t selecteddriver,
    FMOD_INITFLAGS flags,
    int32_t* outputrate,
    int32_t outputchannels,
    FMOD_SOUND_FORMAT* outputformat,
    int32_t dspbufferlength,
    int32_t dspnumbuffers,
    void* extradriverdata) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputALSA::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->close();
}

FMOD_RESULT F_CALLBACK OutputALSA::startCallback(FMOD_OUTPUT_STATE* output) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->start();
}

FMOD_RESULT F_CALLBACK OutputALSA::stopCallback(FMOD_OUTPUT_STATE* output) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->stop();
}

FMOD_RESULT F_CALLBACK OutputALSA::getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->getHandle(handle);
}

FMOD_RESULT F_CALLBACK OutputALSA::recordGetNumDriversCallback(
    FMOD_OUTPUT_STATE* output,
    int32_t* numdrivers) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->recordGetNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputALSA::recordGetDriverInfoCallback(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    char* name,
    int32_t namelen) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->recordGetDriverInfo(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputALSA::recordStartCallback(
    FMOD_OUTPUT_STATE* output,
    int32_t id,
    FMOD_SOUND* sound,
    int32_t loop) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->recordStart(id, sound, loop);
}

FMOD_RESULT F_CALLBACK OutputALSA::recordStopCallback(FMOD_OUTPUT_STATE* output) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->recordStop();
}

FMOD_RESULT F_CALLBACK OutputALSA::recordGetPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->recordGetPosition(pcm);
}

FMOD_RESULT F_CALLBACK OutputALSA::recordLockCallback(
    FMOD_OUTPUT_STATE* output,
    uint32_t offset,
    uint32_t length,
    void** ptr1,
    void** ptr2,
    uint32_t* len1,
    uint32_t* len2) {
    auto alsa = static_cast<OutputALSA*>(output);
    return alsa->recordLock(offset, length, ptr1, ptr2, len1, len2);
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputALSA::getDescriptionEx() {
    FMOD_memset(&alsaoutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    alsaoutput.getnumdrivers = OutputALSA::getNumDriversCallback;
    alsaoutput.getdrivername = OutputALSA::getDriverNameCallback;
    alsaoutput.init = OutputALSA::initCallback;
    alsaoutput.close = OutputALSA::closeCallback;
    alsaoutput.start = OutputALSA::startCallback;
    alsaoutput.stop = OutputALSA::stopCallback;
    alsaoutput.gethandle = OutputALSA::getHandleCallback;

    alsaoutput.record_getnumdrivers = OutputALSA::recordGetNumDriversCallback;
    alsaoutput.record_getdriverinfo = OutputALSA::recordGetDriverInfoCallback;
    alsaoutput.record_start = OutputALSA::recordStartCallback;
    alsaoutput.record_stop = OutputALSA::recordStopCallback;
    alsaoutput.record_getposition = OutputALSA::recordGetPositionCallback;
    alsaoutput.record_lock = OutputALSA::recordLockCallback;

    alsaoutput.mType = FMOD_OUTPUTTYPE_ALSA;
    alsaoutput.mSize = sizeof(OutputALSA);

    return &alsaoutput;
}

} // namespace FMOD
