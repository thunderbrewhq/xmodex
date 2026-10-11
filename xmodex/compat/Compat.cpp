#include "fmod.hpp"
#include "fmod_errors.h"
#include "xmodex/String.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#define COMPAT_TRACE 0

#if COMPAT_TRACE && defined(XMODEX_SYSTEM_WIN)
#include <windows.h>
#endif

enum FMOD_ERRORS {
    _FMOD_NONE,
    _FMOD_BUSY,
    _FMOD_UNINITIALIZED,
    _FMOD_INIT,
    _FMOD_ALLOCATED,
    _FMOD_PLAY,
    _FMOD_OUTPUT_FORMAT,
    _FMOD_COOPERATIVELEVEL,
    _FMOD_CREATEBUFFER,
    _FMOD_FILE_NOTFOUND,
    _FMOD_FILE_FORMAT,
    _FMOD_FILE_BAD,
    _FMOD_MEMORY,
    _FMOD_VERSION,
    _FMOD_INVALID_PARAM,
    _FMOD_NO_EAX,
    _FMOD_CHANNEL_ALLOC,
    _FMOD_RECORD,
    _FMOD_MEDIAPLAYER,
    _FMOD_CDDEVICE
};

enum FSOUND_OUTPUTTYPES {
    FSOUND_OUTPUT_NOSOUND,
    FSOUND_OUTPUT_WINMM,
    FSOUND_OUTPUT_DSOUND,
    FSOUND_OUTPUT_A3D,
    FSOUND_OUTPUT_OSS,
    FSOUND_OUTPUT_ESD,
    FSOUND_OUTPUT_ALSA,
    FSOUND_OUTPUT_ASIO,
    FSOUND_OUTPUT_XBOX,
    FSOUND_OUTPUT_PS2,
    FSOUND_OUTPUT_MAC,
    FSOUND_OUTPUT_GC,
    FSOUND_OUTPUT_PSP,
    FSOUND_OUTPUT_NOSOUND_NONREALTIME
};

#define FSOUND_LOOP_OFF 0x00000001
#define FSOUND_LOOP_NORMAL 0x00000002
#define FSOUND_LOOP_BIDI 0x00000004
#define FSOUND_8BITS 0x00000008
#define FSOUND_16BITS 0x00000010
#define FSOUND_MONO 0x00000020
#define FSOUND_STEREO 0x00000040
#define FSOUND_UNSIGNED 0x00000080
#define FSOUND_SIGNED 0x00000100
#define FSOUND_DELTA 0x00000200
#define FSOUND_IT214 0x00000400
#define FSOUND_IT215 0x00000800
#define FSOUND_HW3D 0x00001000
#define FSOUND_2D 0x00002000
#define FSOUND_STREAMABLE 0x00004000
#define FSOUND_LOADMEMORY 0x00008000
#define FSOUND_LOADRAW 0x00010000
#define FSOUND_MPEGACCURATE 0x00020000
#define FSOUND_FORCEMONO 0x00040000
#define FSOUND_HW2D 0x00080000
#define FSOUND_ENABLEFX 0x00100000
#define FSOUND_MPEGHALFRATE 0x00200000
#define FSOUND_IMAADPCM 0x00400000
#define FSOUND_VAG 0x00800000
#define FSOUND_NONBLOCKING 0x01000000
#define FSOUND_GCADPCM 0x02000000
#define FSOUND_MULTICHANNEL 0x04000000
#define FSOUND_USECORE0 0x08000000
#define FSOUND_USECORE1 0x10000000
#define FSOUND_LOADMEMORYIOP 0x20000000
#define FSOUND_IGNORETAGS 0x40000000
#define FSOUND_STREAM_NET 0x80000000

#define FSOUND_NORMAL (FSOUND_16BITS | FSOUND_SIGNED | FSOUND_MONO)

struct FSOUND_STREAM;
struct FSOUND_SAMPLE;

struct FSOUND_REVERB_PROPERTIES {
        uint32_t Environment;
        float EnvSize;
        float EnvDiffusion;
        int32_t Room;
        int32_t RoomHF;
        int32_t RoomLF;
        float DecayTime;
        float DecayHFRatio;
        float DecayLFRatio;
        int32_t Reflections;
        float ReflectionsDelay;
        float ReflectionsPan[3];
        int32_t Reverb;
        float ReverbDelay;
        float ReverbPan[3];
        float EchoTime;
        float EchoDepth;
        float ModulationTime;
        float ModulationDepth;
        float AirAbsorptionHF;
        float HFReference;
        float LFReference;
        float RoomRolloffFactor;
        float Diffusion;
        float Density;
        uint32_t Flags;
};

typedef int8_t(F_CALLBACK* FSOUND_STREAMCALLBACK)(FSOUND_STREAM*, void*, int32_t, int32_t);
typedef void*(F_CALLBACK* FSOUND_ALLOCCALLBACK)(uint32_t);
typedef void*(F_CALLBACK* FSOUND_REALLOCCALLBACK)(void*, uint32_t);
typedef void(F_CALLBACK* FSOUND_FREECALLBACK)(void*);
typedef uint32_t(F_CALLBACK* FSOUND_OPENCALLBACK)(const char*);
typedef void(F_CALLBACK* FSOUND_CLOSECALLBACK)(uint32_t);
typedef int32_t(F_CALLBACK* FSOUND_READCALLBACK)(void*, int32_t, uint32_t);
typedef int32_t(F_CALLBACK* FSOUND_SEEKCALLBACK)(uint32_t, int32_t, int8_t);
typedef int32_t(F_CALLBACK* FSOUND_TELLCALLBACK)(uint32_t);

struct compat_system {
        bool initialized = false;
        FMOD_RESULT error = FMOD_OK;
        FMOD::System* system = nullptr;
        // 3d settings
        float dopplerscale = 1.0f;
        float distancefactor = 1.0f;
        float rolloffscale = 1.0f;
        // filesystem callbacks
        FSOUND_OPENCALLBACK opencallback = nullptr;
        FSOUND_CLOSECALLBACK closecallback = nullptr;
        FSOUND_READCALLBACK readcallback = nullptr;
        FSOUND_SEEKCALLBACK seekcallback = nullptr;
        FSOUND_TELLCALLBACK tellcallback = nullptr;
        // allocator callbacks
        FSOUND_ALLOCCALLBACK alloccallback = nullptr;
        FSOUND_REALLOCCALLBACK realloccallback = nullptr;
        FSOUND_FREECALLBACK freecallback = nullptr;
        // hacks
        FSOUND_STREAM* pendingstream = nullptr;
        FSOUND_STREAMCALLBACK pendingcallback = nullptr;
        int32_t pendinguserdata = 0;
        FSOUND_STREAM* channelsounds[4096] = { 0 };
        FSOUND_STREAMCALLBACK channelcallbacks[4096] = { 0 };
        int32_t channeluserdata[4096] = { 0 };
};

struct compat_stream {
        FMOD::Channel* channel;
        FMOD::Sound* sound;
};

compat_system g;

void compat_tracef(const char* format, ...) {
#if COMPAT_TRACE && defined(XMODEX_SYSTEM_WIN)

    static char buffer[2048];

    va_list arg;
    va_start(arg, format);

    vsnprintf(buffer, 2048, format, arg);

    OutputDebugStringA(buffer);

#endif
}

// allocator callbacks

void* F_CALLBACK compat_alloc(int32_t size, FMOD_MEMORY_TYPE type) {
    if (g.alloccallback) {
        return g.alloccallback(size);
    }

    return malloc(size);
}

void* F_CALLBACK compat_realloc(void* ptr, int32_t size, FMOD_MEMORY_TYPE type) {
    if (g.realloccallback) {
        return g.realloccallback(ptr, size);
    }

    return realloc(ptr, size);
}

void F_CALLBACK compat_free(void* ptr, FMOD_MEMORY_TYPE type) {
    if (g.freecallback) {
        g.freecallback(ptr);
        return;
    }

    return free(ptr);
}

// filesystem callbacks

FMOD_RESULT F_CALLBACK compat_open(
    const char* name,
    int32_t unicode,
    uint32_t* filesize,
    void** handle,
    void** userdata) {

    if (g.opencallback) {
        uint32_t handle_;
        handle_ = g.opencallback(name);
        if (!handle_) {
            compat_tracef("compat_open: underlying open callback failed to open sound file \"%s\"", name);
            return FMOD_ERR_FILE_NOTFOUND;
        }

        *filesize = static_cast<uint32_t>(g.seekcallback(handle_, 0, 2));
        compat_tracef("\"%s\" is %d bytes", name, *filesize);
        g.seekcallback(handle_, 0, 0);

        *handle = reinterpret_cast<void*>(handle_);
        return FMOD_OK;
    }

    compat_tracef("compat_open: underlying open callback was never set!");

    return FMOD_ERR_FILE_BAD;
}

FMOD_RESULT F_CALLBACK compat_close(
    void* handle,
    void* userdata) {

    if (g.closecallback) {
        g.closecallback(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(handle)));
        return FMOD_OK;
    }

    return FMOD_ERR_FILE_BAD;
}

FMOD_RESULT F_CALLBACK compat_read(
    void* handle,
    void* buffer,
    uint32_t sizebytes,
    uint32_t* bytesread,
    void* userdata) {

    if (g.readcallback) {
        *bytesread = static_cast<uint32_t>(g.readcallback(buffer, static_cast<int32_t>(sizebytes), static_cast<uint32_t>(reinterpret_cast<uintptr_t>(handle))));
        if (!*bytesread) {
            return FMOD_ERR_FILE_EOF;
        }

        return FMOD_OK;
    }

    return FMOD_ERR_FILE_BAD;
}

FMOD_RESULT F_CALLBACK compat_seek(
    void* handle,
    uint32_t pos,
    void* userdata) {

    if (g.seekcallback) {
        g.seekcallback(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(handle)),
            static_cast<int32_t>(pos),
            0);
        return FMOD_OK;
    }

    return FMOD_ERR_FILE_BAD;
}

// channel end callback hack

FMOD_RESULT F_CALLBACK compat_end_callback(
    FMOD_CHANNEL* channel,
    FMOD_CHANNEL_CALLBACKTYPE type,
    int32_t command,
    uint32_t commanddata1,
    uint32_t commanddata2) {

    auto channelhandle = reinterpret_cast<uintptr_t>(channel);
    auto channelindex = static_cast<int32_t>((channelhandle >> 16) & 0xFFF);

    compat_tracef("compat callback invoked for channel %d", channelindex);

    auto callback = g.channelcallbacks[channelindex];
    auto userdata = g.channeluserdata[channelindex];
    auto stream = g.channelsounds[channelindex];

    g.channelcallbacks[channelindex] = nullptr;
    g.channeluserdata[channelindex] = 0;
    g.channelsounds[channelindex] = nullptr;

    if (callback) {
        compat_tracef("calling actual end sound callback for channel %d", channelindex);
        callback(stream, nullptr, 0, userdata);
    }

    return FMOD_OK;
}

extern "C" void F_API _FSOUND_3D_SetDistanceFactor(float factor) {
    if (g.distancefactor != factor) {
        g.distancefactor = factor;
        if (g.system) {
            g.system->set3DSettings(g.dopplerscale, g.distancefactor, g.rolloffscale);
        }
    }
}

extern "C" void F_API _FSOUND_3D_SetDopplerFactor(float factor) {
    if (g.dopplerscale != factor) {
        g.dopplerscale = factor;
        if (g.system) {
            g.system->set3DSettings(g.dopplerscale, g.distancefactor, g.rolloffscale);
        }
    }
}

extern "C" void F_API _FSOUND_3D_SetRolloffFactor(float factor) {
    if (g.rolloffscale != factor) {
        g.rolloffscale = factor;
        if (g.system) {
            g.system->set3DSettings(g.dopplerscale, g.distancefactor, g.rolloffscale);
        }
    }
}

extern "C" float F_API _FSOUND_GetCPUUsage() {
    // TODO
    return 0.0f;
}

extern "C" int8_t F_API _FSOUND_3D_Listener_SetAttributes(
    const float* position,
    const float* velocity,
    float forwardX,
    float forwardY,
    float forwardZ,
    float topX,
    float topY,
    float topZ) {

    FMOD_RESULT result;
    FMOD_VECTOR pos;
    FMOD_VECTOR vel;
    FMOD_VECTOR forward;
    FMOD_VECTOR top;

    if (position) {
        pos.x = position[0];
        pos.y = position[1];
        pos.z = position[2];
    }

    if (velocity) {
        vel.x = velocity[0];
        vel.y = velocity[1];
        vel.z = velocity[2];
    }

    forward.x = forwardX;
    forward.y = forwardY;
    forward.z = forwardZ;

    top.x = topX;
    top.y = topY;
    top.z = topZ;

    if (g.system) {
        result = g.system->set3DListenerAttributes(
            0,
            position ? &pos : nullptr,
            velocity ? &vel : nullptr,
            &forward,
            &top);
        if (result != FMOD_OK) {
            return 0;
        }

        return 1;
    }

    return 0;
}

extern "C" int8_t F_API _FSOUND_3D_SetAttributes(int32_t channel, const float* position, const float* velocity) {
    // g.create();

    FMOD_VECTOR pos;
    FMOD_VECTOR vel;

    if (position) {
        pos.x = position[0];
        pos.y = position[1];
        pos.z = position[2];
    }

    if (velocity) {
        vel.x = velocity[0];
        vel.y = velocity[1];
        vel.z = velocity[2];
    }

    if (g.system) {
        FMOD_RESULT result;
        FMOD::Channel* channel_;
        result = g.system->getChannel(channel, &channel_);
        if (result != FMOD_OK) {
            return 0;
        }

        result = channel_->set3DAttributes(
            position ? &pos : nullptr,
            velocity ? &vel : nullptr);
        if (result != FMOD_OK) {
            return 0;
        }

        return 1;
    }

    return 0;
}

extern "C" void F_API _FSOUND_3D_Listener_GetAttributes(
    float* position,
    float* velocity,
    float* forwardX,
    float* forwardY,
    float* forwardZ,
    float* topX,
    float* topY,
    float* topZ) {

    // g.create();

    if (g.system) {
        FMOD_VECTOR pos;
        FMOD_VECTOR vel;
        FMOD_VECTOR forward;
        FMOD_VECTOR up;
        g.system->get3DListenerAttributes(
            0,
            &pos,
            &vel,
            &forward,
            &up);

        if (position) {
            position[0] = pos.x;
            position[1] = pos.y;
            position[2] = pos.z;
        }

        if (velocity) {
            velocity[0] = vel.x;
            velocity[1] = vel.y;
            velocity[2] = vel.z;
        }

        if (forwardX) {
            *forwardX = forward.x;
        }
        if (forwardY) {
            *forwardY = forward.y;
        }
        if (forwardZ) {
            *forwardZ = forward.z;
        }

        if (topX) {
            *topX = up.x;
        }
        if (topY) {
            *topY = up.y;
        }
        if (topZ) {
            *topZ = up.z;
        }
    }
}

extern "C" int8_t F_API _FSOUND_IsPlaying(int32_t channel) {
    if (g.system) {
        FMOD_RESULT result;
        FMOD::Channel* channel_;
        result = g.system->getChannel(channel, &channel_);
        if (result != FMOD_OK) {
            return 0;
        }

        bool isplaying;
        result = channel_->isPlaying(&isplaying);
        if (result != FMOD_OK) {
            return 0;
        }

        if (isplaying) {
            return 1;
        }
    }

    return 0;
}

extern "C" void F_API _FSOUND_Close() {
    if (g.system) {
        g.system->release();
        g.system = nullptr;
    }
}

extern "C" void F_API _FSOUND_File_SetCallbacks(
    FSOUND_OPENCALLBACK openCallback,
    FSOUND_CLOSECALLBACK closeCallback,
    FSOUND_READCALLBACK readCallback,
    FSOUND_SEEKCALLBACK seekCallback,
    FSOUND_TELLCALLBACK tellCallback) {

    g.opencallback = openCallback;
    g.closecallback = closeCallback;
    g.readcallback = readCallback;
    g.seekcallback = seekCallback;
    g.tellcallback = tellCallback;
}

extern "C" int32_t F_API _FSOUND_GetDriver() {
    // g.create();

    int32_t driver = 0;

    if (g.system) {
        g.system->getDriver(&driver);
    }

    return driver;
}

extern "C" int8_t F_API _FSOUND_GetDriverCaps(int32_t driver, uint32_t* caps) {
    // TODO
    if (caps) {
        *caps = 0;
    }
    return 1;
}

extern "C" const char* F_API _FSOUND_GetDriverName(int32_t driver) {
    // g.create();

    static char name[256];

    if (g.system) {
        g.system->getDriverName(driver, name, 256);
    }

    return name;
}

extern "C" int32_t F_API _FSOUND_GetError() {
    // TODO:
    if (g.error != FMOD_OK) {
        return _FMOD_INVALID_PARAM;
    }
    return 0;
}

extern "C" int32_t F_API _FSOUND_GetMaxChannels() {
    int32_t maxchannels = 0;

    if (g.system) {
        g.system->getSoftwareChannels(&maxchannels);
    }

    return maxchannels;
}

extern "C" int32_t F_API _FSOUND_GetMixer() {
    // TODO
    return 0;
}

extern "C" int32_t F_API _FSOUND_GetNumDrivers() {
    // g.create();

    int32_t numdrivers = 0;

    if (g.system) {
        g.system->getNumDrivers(&numdrivers);
    }

    return numdrivers;
}

extern "C" int32_t F_API _FSOUND_GetNumHardwareChannels() {
    int32_t num3d = 0;

    if (g.system) {
        g.system->getHardwareChannels(nullptr, &num3d, nullptr);
    }

    return num3d;
}

extern "C" int32_t F_API _FSOUND_GetOutput() {
    FMOD_OUTPUTTYPE outputtype4 = FMOD_OUTPUTTYPE_AUTODETECT;
    int32_t outputtype3 = FSOUND_OUTPUT_NOSOUND;

    if (g.system) {
        FMOD_RESULT result;
        result = g.system->getOutput(&outputtype4);
        if (result == FMOD_OK) {
            switch (outputtype4) {
            case FMOD_OUTPUTTYPE_UNKNOWN:
            case FMOD_OUTPUTTYPE_NOSOUND:
                outputtype3 = FSOUND_OUTPUT_NOSOUND;
                break;
            case FMOD_OUTPUTTYPE_NOSOUND_NRT:
                outputtype3 = FSOUND_OUTPUT_NOSOUND_NONREALTIME;
                break;
            case FMOD_OUTPUTTYPE_DSOUND:
                outputtype3 = FSOUND_OUTPUT_DSOUND;
                break;
            case FMOD_OUTPUTTYPE_WINMM:
            case FMOD_OUTPUTTYPE_WASAPI:
                outputtype3 = FSOUND_OUTPUT_WINMM;
            case FMOD_OUTPUTTYPE_ASIO:
                outputtype3 = FSOUND_OUTPUT_ASIO;
                break;
            case FMOD_OUTPUTTYPE_OSS:
                outputtype3 = FSOUND_OUTPUT_OSS;
                break;
            case FMOD_OUTPUTTYPE_ALSA:
                outputtype3 = FSOUND_OUTPUT_ALSA;
                break;
            case FMOD_OUTPUTTYPE_ESD:
                outputtype3 = FSOUND_OUTPUT_ESD;
                break;
            case FMOD_OUTPUTTYPE_SOUNDMANAGER:
            case FMOD_OUTPUTTYPE_COREAUDIO:
                outputtype3 = FSOUND_OUTPUT_MAC;
                break;
            case FMOD_OUTPUTTYPE_XBOX:
                outputtype3 = FSOUND_OUTPUT_XBOX;
                break;
            case FMOD_OUTPUTTYPE_PS2:
                outputtype3 = FSOUND_OUTPUT_PS2;
                break;
            case FMOD_OUTPUTTYPE_GC:
                outputtype3 = FSOUND_OUTPUT_GC;
                break;
            default:
                outputtype3 = FSOUND_OUTPUT_NOSOUND;
                break;
            }
        }
    }

    return outputtype3;
}

extern "C" int32_t F_API _FSOUND_GetOutputRate() {
    int32_t samplerate = 44100;

    if (g.system) {
        g.system->getSoftwareFormat(&samplerate, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    return samplerate;
}

extern "C" int8_t F_API _FSOUND_Init(int32_t mixRate, int32_t maxSoftwareChannels, uint32_t flags) {
    if (!g.system) {
        FMOD_RESULT result;

        result = FMOD::System_Create(&g.system);
        if (result != FMOD_OK) {
            g.error = result;
            return 0;
        }

        result = g.system->init(maxSoftwareChannels, FMOD_INIT_NORMAL, nullptr);
        if (result == FMOD_OK) {
            return 1;
        }
    }

    return 0;
}

extern "C" int8_t F_API _FSOUND_Sample_SetMinMaxDistance(FSOUND_SAMPLE* sample, float minDistance, float maxDistance) {
    FMOD_RESULT result;

    auto sound = reinterpret_cast<FMOD::Sound*>(sample);
    if (!sound) {
        return 0;
    }

    result = sound->set3DMinMaxDistance(minDistance, maxDistance);
    if (result != FMOD_OK) {
        return 0;
    }

    return 1;
}

extern "C" int8_t F_API _FSOUND_SetMute(int32_t channel, int8_t mute) {
    if (g.system) {
        FMOD_RESULT result;
        FMOD::Channel* channel_;

        result = g.system->getChannel(channel, &channel_);
        if (result != FMOD_OK) {
            return 0;
        }

        result = channel_->setMute(mute == 1 ? true : false);
        if (result != FMOD_OK) {
            return 0;
        }

        return 1;
    }

    return 0;
}

extern "C" int8_t F_API _FSOUND_SetPaused(int32_t channel, int8_t paused) {
    if (g.system) {
        FMOD_RESULT result;
        FMOD::Channel* channel_;

        result = g.system->getChannel(channel, &channel_);
        if (result != FMOD_OK) {
            return 0;
        }

        result = channel_->setPaused(paused == 1 ? true : false);
        if (result != FMOD_OK) {
            return 0;
        }

        return 1;
    }

    return 0;
}

extern "C" int8_t F_API _FSOUND_SetFrequency(int32_t channel, int32_t frequency) {
    // if (g.system) {
    //     FMOD_RESULT result;
    //     FMOD::Channel* channel_;
    //
    //     result = g.system->getChannel(channel, &channel_);
    //     if (result != FMOD_OK) {
    //         return 0;
    //     }
    //
    //     result = channel_->setFrequency(static_cast<float>(frequency));
    //     if (result != FMOD_OK) {
    //         return 0;
    //     }
    //
    //     return 1;
    // }
    //
    // return 0;

    return 1;
}

extern "C" int8_t F_API _FSOUND_SetBufferSize(int32_t milliseconds) {
    // TODO
    return 1;
}

extern "C" int8_t F_API _FSOUND_SetDriver(int32_t driver) {
    // if (g.system) {
    //     FMOD_RESULT result;
    //
    //     result = g.system->setDriver(driver);
    //     if (result != FMOD_OK) {
    //         return 0;
    //     }
    //
    //     return 1;
    // }
    //
    // return 0;
    return 1;
}

extern "C" int8_t F_API _FSOUND_SetHWND(void* window) {
    // TODO
    return 1;
}

extern "C" int8_t F_API _FSOUND_SetMaxHardwareChannels(int32_t maximum) {
    // if (g.system) {
    //     // TODO
    //     g.system->setHardwareChannels(maximum, maximum, maximum, maximum);
    // }

    return 1;
}

extern "C" int8_t F_API _FSOUND_SetMemorySystem(
    void* pool,
    int32_t poolLength,
    FSOUND_ALLOCCALLBACK allocCallback,
    FSOUND_REALLOCCALLBACK reallocCallback,
    FSOUND_FREECALLBACK freeCallback) {

    g.alloccallback = allocCallback;
    g.realloccallback = reallocCallback;
    g.freecallback = freeCallback;

    FMOD::Memory_Initialize(
        pool,
        poolLength,
        compat_alloc,
        compat_realloc,
        compat_free);

    return 1;
}

extern "C" int8_t F_API _FSOUND_SetMinHardwareChannels(int32_t minimum) {
    // TODO
    return 1;
}

extern "C" int8_t F_API _FSOUND_SetMixer(int32_t mixer) {
    // TODO
    return 1;
}

extern "C" int8_t F_API _FSOUND_SetOutput(int32_t output) {
    // TODO
    return 1;
}

extern "C" int8_t F_API _FSOUND_SetSFXMasterVolume(int32_t volume) {
    // g.create();

    float level = static_cast<float>(volume) / 255.0f;

    if (g.system) {
        FMOD_RESULT result;
        FMOD::ChannelGroup* mastergroup;

        result = g.system->getMasterChannelGroup(&mastergroup);
        if (result == FMOD_OK) {
            result = mastergroup->setVolume(level);
            if (result == FMOD_OK) {
                return 1;
            }
        }
    }

    return 0;
}

extern "C" int8_t F_API _FSOUND_SetVolume(int32_t channel, int32_t volume) {
    float level = static_cast<float>(volume) / 255.0f;

    if (g.system) {
        FMOD_RESULT result;
        FMOD::Channel* channel_;

        result = g.system->getChannel(channel, &channel_);
        if (result == FMOD_OK) {
            result = channel_->setVolume(level);
            if (result == FMOD_OK) {
                return 1;
            }
        }
    }

    return 0;
}

extern "C" int8_t F_API _FSOUND_StopSound(int32_t channel) {
    if (g.system) {
        FMOD_RESULT result;
        FMOD::Channel* channel_;

        result = g.system->getChannel(channel, &channel_);
        if (result == FMOD_OK) {
            compat_tracef("stopping sound");
            result = channel_->stop();
            if (result == FMOD_OK) {
                return 1;
            }
        }
    }

    return 0;
}

extern "C" int8_t F_API _FSOUND_Stream_Close(FSOUND_STREAM* stream) {
    auto sound = reinterpret_cast<FMOD::Sound*>(stream);
    if (sound) {
        sound->release();
        return 1;
    }

    return 0;
}

extern "C" FSOUND_STREAM* F_API _FSOUND_Stream_Open(const char* filename, uint32_t mode, int32_t offset, int32_t length) {
    FMOD_CREATESOUNDEXINFO exinfo;
    FMOD_memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

    if (g.opencallback && g.closecallback && g.readcallback && g.seekcallback) {
        exinfo.useropen = compat_open;
        exinfo.userclose = compat_close;
        exinfo.userread = compat_read;
        exinfo.userseek = compat_seek;
    }

    if (g.system) {
        FMOD_RESULT result;
        FMOD::Sound* sound;

        uint32_t mode4 = FMOD_SOFTWARE;
        if (mode & FSOUND_LOOP_OFF) {
            mode4 |= FMOD_LOOP_OFF;
        }
        if (mode & FSOUND_LOOP_NORMAL) {
            mode4 |= FMOD_LOOP_NORMAL;
        }
        if (mode & FSOUND_LOOP_BIDI) {
            mode4 |= FMOD_LOOP_BIDI;
        }

        result = g.system->createSound(filename, mode4, &exinfo, &sound);
        if (result != FMOD_OK) {
            compat_tracef("underlying call to createSound() failed: %s", filename);
            compat_tracef("%d: %s", result, FMOD_ErrorString(result));
            return nullptr;
        }
        compat_tracef("created sound %s\n", filename);

        return reinterpret_cast<FSOUND_STREAM*>(sound);
    }

    return nullptr;
}

extern "C" FSOUND_SAMPLE* F_API _FSOUND_Stream_GetSample(FSOUND_STREAM* stream) {
    return reinterpret_cast<FSOUND_SAMPLE*>(stream);
}

extern "C" int32_t F_API _FSOUND_Stream_PlayEx(int32_t channel, FSOUND_STREAM* stream, void* dsp, int8_t startPaused) {
    // this hack doesn't allow you to request a custom channel, sadly..
    if (channel != -1) {
        return -1;
    }

    if (g.system) {
        FMOD_RESULT result;
        FMOD::Channel* channel_ = nullptr;

        compat_tracef("playing sound...");

        result = g.system->playSound(FMOD_CHANNEL_FREE, reinterpret_cast<FMOD::Sound*>(stream), startPaused == 1 ? true : false, &channel_);
        if (result != FMOD_OK) {
            return -1;
        }

        if (channel_ == nullptr) {
            compat_tracef("could not obtain handle to freechannel?");
        }

        compat_tracef("playSound worked");

        // hack to obtain the index of the channel
        auto channelhandle = reinterpret_cast<uintptr_t>(channel_);
        auto channelindex = static_cast<int32_t>((channelhandle >> 16) & 0xFFF);

        if (g.pendingstream == stream) {
            compat_tracef("setting callback");
            channel_->setCallback(FMOD_CHANNEL_CALLBACKTYPE_END, compat_end_callback, 0);
            compat_tracef("set callback");
            g.channelsounds[channelindex] = stream;
            g.channelcallbacks[channelindex] = g.pendingcallback;
            g.channeluserdata[channelindex] = g.pendinguserdata;
            g.pendingstream = nullptr;
            g.pendingcallback = nullptr;
            g.pendinguserdata = 0;
        }

        return channelindex;
    }

    return -1;
}

extern "C" int8_t F_API _FSOUND_Stream_SetEndCallback(FSOUND_STREAM* stream, FSOUND_STREAMCALLBACK callback, int32_t userdata) {
    g.pendingstream = stream;
    g.pendingcallback = callback;
    g.pendinguserdata = userdata;

    return 1;
}

extern "C" int8_t F_API _FSOUND_Stream_SetLoopCount(FSOUND_STREAM* stream, int32_t loopCount) {
    auto sound = reinterpret_cast<FMOD::Sound*>(stream);
    if (sound) {
        FMOD_RESULT result;
        result = sound->setLoopCount(loopCount);
        if (result != FMOD_OK) {
            return 0;
        }
        return 1;
    }
    return 0;
}

extern "C" int32_t F_API _FSOUND_Stream_GetLengthMs(FSOUND_STREAM* stream) {
    uint32_t length = 0;
    auto sound = reinterpret_cast<FMOD::Sound*>(stream);
    if (sound) {
        FMOD_RESULT result;
        result = sound->getLength(&length, FMOD_TIMEUNIT_MS);
        if (result != FMOD_OK) {
            // TODO
        }
    }
    return length;
}

extern "C" int8_t F_API _FSOUND_Stream_SetTime(FSOUND_STREAM* stream, int32_t milliseconds) {
    // TODO
    return 0;
}

extern "C" int8_t F_API _FSOUND_Stream_Stop(FSOUND_STREAM* stream) {
    // TODO
    return 1;
}

extern "C" void F_API _FSOUND_Update() {
    if (g.system) {
        g.system->update();
    }
}
