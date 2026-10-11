#ifndef FMOD_HPP
#define FMOD_HPP

#include <cstdint>

#define FMOD_VERSION 0x40907

#define FMOD_CAPS_NONE 0x00000000
#define FMOD_CAPS_HARDWARE 0x00000001
#define FMOD_CAPS_HARDWARE_EMULATED 0x00000002
#define FMOD_CAPS_OUTPUT_MULTICHANNEL 0x00000004
#define FMOD_CAPS_OUTPUT_FORMAT_PCM8 0x00000008
#define FMOD_CAPS_OUTPUT_FORMAT_PCM16 0x00000010
#define FMOD_CAPS_OUTPUT_FORMAT_PCM24 0x00000020
#define FMOD_CAPS_OUTPUT_FORMAT_PCM32 0x00000040
#define FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT 0x00000080
#define FMOD_CAPS_REVERB_EAX2 0x00000100
#define FMOD_CAPS_REVERB_EAX3 0x00000200
#define FMOD_CAPS_REVERB_EAX4 0x00000400
#define FMOD_CAPS_REVERB_I3DL2 0x00000800
#define FMOD_CAPS_REVERB_LIMITED 0x00001000

#define FMOD_INIT_NORMAL 0x00000000
#define FMOD_INIT_STREAM_FROM_UPDATE 0x00000001
#define FMOD_INIT_3D_RIGHTHANDED 0x00000002
#define FMOD_INIT_DISABLESOFTWARE 0x00000004
#define FMOD_INIT_SOFTWARE_OCCLUSION 0x00000008
#define FMOD_INIT_SOFTWARE_HRTF 0x00000010
#define FMOD_INIT_ENABLE_DSPNET 0x00000020
#define FMOD_INIT_VOL0_BECOMES_VIRTUAL 0x00000080
#define FMOD_INIT_WASAPI_EXCLUSIVE 0x00000100
#define FMOD_INIT_DSOUND_HRTFNONE 0x00000200
#define FMOD_INIT_DSOUND_HRTFLIGHT 0x00000400
#define FMOD_INIT_DSOUND_HRTFFULL 0x00000800
#define FMOD_INIT_PS2_DISABLECORE0REVERB 0x00010000
#define FMOD_INIT_PS2_DISABLECORE1REVERB 0x00020000
#define FMOD_INIT_PS2_DONTUSESCRATCHPAD 0x00040000
#define FMOD_INIT_XBOX_REMOVEHEADROOM 0x00100000
#define FMOD_INIT_PS3_PREFERDTS 0x00800000
#define FMOD_INIT_PS3_FORCE2CHLPCM 0x01000000
#define FMOD_INIT_DISABLEDOLBY 0x00100000
#define FMOD_INIT_SYSTEM_MUSICMUTENOTPAUSE 0x00200000
#define FMOD_INIT_SYNCMIXERWITHUPDATE 0x00400000
#define FMOD_INIT_GEOMETRY_USECLOSEST 0x04000000
#define FMOD_INIT_DISABLE_MYEARS_AUTODETECT 0x08000000
#define FMOD_INIT_PS3_DISABLEDTS 0x10000000
#define FMOD_INIT_PS3_DISABLEDOLBYDIGITAL 0x20000000

#define FMOD_DEFAULT 0x00000000
#define FMOD_LOOP_OFF 0x00000001
#define FMOD_LOOP_NORMAL 0x00000002
#define FMOD_LOOP_BIDI 0x00000004
#define FMOD_2D 0x00000008
#define FMOD_3D 0x00000010
#define FMOD_HARDWARE 0x00000020
#define FMOD_SOFTWARE 0x00000040
#define FMOD_CREATESTREAM 0x00000080
#define FMOD_CREATESAMPLE 0x00000100
#define FMOD_CREATECOMPRESSEDSAMPLE 0x00000200
#define FMOD_OPENUSER 0x00000400
#define FMOD_OPENMEMORY 0x00000800
#define FMOD_OPENRAW 0x00001000
#define FMOD_OPENONLY 0x00002000
#define FMOD_ACCURATETIME 0x00004000
#define FMOD_MPEGSEARCH 0x00008000
#define FMOD_NONBLOCKING 0x00010000
#define FMOD_UNIQUE 0x00020000
#define FMOD_3D_HEADRELATIVE 0x00040000
#define FMOD_3D_WORLDRELATIVE 0x00080000
#define FMOD_3D_LOGROLLOFF 0x00100000
#define FMOD_3D_LINEARROLLOFF 0x00200000
#define FMOD_3D_CUSTOMROLLOFF 0x04000000
#define FMOD_3D_IGNOREGEOMETRY 0x40000000
#define FMOD_CDDA_FORCEASPI 0x00400000
#define FMOD_CDDA_JITTERCORRECT 0x00800000
#define FMOD_UNICODE 0x01000000
#define FMOD_IGNORETAGS 0x02000000
#define FMOD_LOWMEM 0x08000000
#define FMOD_OPENMEMORY_POINT 0x10000000
#define FMOD_VIRTUAL_PLAYFROMSTART 0x80000000

#define FMOD_TIMEUNIT_MS 0x00000001
#define FMOD_TIMEUNIT_PCM 0x00000002
#define FMOD_TIMEUNIT_PCMBYTES 0x00000004
#define FMOD_TIMEUNIT_RAWBYTES 0x00000008
#define FMOD_TIMEUNIT_MODORDER 0x00000100
#define FMOD_TIMEUNIT_MODROW 0x00000200
#define FMOD_TIMEUNIT_MODPATTERN 0x00000400
#define FMOD_TIMEUNIT_SENTENCE_MS 0x00010000
#define FMOD_TIMEUNIT_SENTENCE_PCM 0x00020000
#define FMOD_TIMEUNIT_SENTENCE_PCMBYTES 0x00040000
#define FMOD_TIMEUNIT_SENTENCE 0x00080000
#define FMOD_TIMEUNIT_SENTENCE_SUBSOUND 0x00100000
#define FMOD_TIMEUNIT_BUFFERED 0x10000000

#define FMOD_REVERB_FLAGS_DECAYTIMESCALE 0x00000001
#define FMOD_REVERB_FLAGS_REFLECTIONSSCALE 0x00000002
#define FMOD_REVERB_FLAGS_REFLECTIONSDELAYSCALE 0x00000004
#define FMOD_REVERB_FLAGS_REVERBSCALE 0x00000008
#define FMOD_REVERB_FLAGS_REVERBDELAYSCALE 0x00000010
#define FMOD_REVERB_FLAGS_DECAYHFLIMIT 0x00000020
#define FMOD_REVERB_FLAGS_ECHOTIMESCALE 0x00000040
#define FMOD_REVERB_FLAGS_MODULATIONTIMESCALE 0x00000080
#define FMOD_REVERB_FLAGS_CORE0 0x00000100
#define FMOD_REVERB_FLAGS_CORE1 0x00000200
#define FMOD_REVERB_FLAGS_HIGHQUALITYREVERB 0x00000400

#define FMOD_REVERB_FLAGS_DEFAULT (FMOD_REVERB_FLAGS_DECAYTIMESCALE |        \
                                   FMOD_REVERB_FLAGS_REFLECTIONSSCALE |      \
                                   FMOD_REVERB_FLAGS_REFLECTIONSDELAYSCALE | \
                                   FMOD_REVERB_FLAGS_REVERBSCALE |           \
                                   FMOD_REVERB_FLAGS_REVERBDELAYSCALE |      \
                                   FMOD_REVERB_FLAGS_DECAYHFLIMIT |          \
                                   FMOD_REVERB_FLAGS_CORE0 |                 \
                                   FMOD_REVERB_FLAGS_CORE1)

#define FMOD_REVERB_CHANNELFLAGS_DIRECTHFAUTO 0x00000001
#define FMOD_REVERB_CHANNELFLAGS_ROOMAUTO 0x00000002
#define FMOD_REVERB_CHANNELFLAGS_ROOMHFAUTO 0x00000004
#define FMOD_REVERB_CHANNELFLAGS_ENVIRONMENT0 0x00000008
#define FMOD_REVERB_CHANNELFLAGS_ENVIRONMENT1 0x00000010
#define FMOD_REVERB_CHANNELFLAGS_ENVIRONMENT2 0x00000020

#define FMOD_REVERB_CHANNELFLAGS_DEFAULT (FMOD_REVERB_CHANNELFLAGS_DIRECTHFAUTO | \
                                          FMOD_REVERB_CHANNELFLAGS_ROOMAUTO |     \
                                          FMOD_REVERB_CHANNELFLAGS_ROOMHFAUTO |   \
                                          FMOD_REVERB_CHANNELFLAGS_ENVIRONMENT0)

#define FMOD_MEMORY_NORMAL 0x00000000
#define FMOD_MEMORY_STREAM_FILE 0x00000001
#define FMOD_MEMORY_STREAM_DECODE 0x00000002
#define FMOD_MEMORY_SAMPLEDATA 0x00000004
#define FMOD_MEMORY_DSP_OUTPUTBUFFER 0x00000008
#define FMOD_MEMORY_XBOX360_PHYSICAL 0x00100000
#define FMOD_MEMORY_PERSISTENT 0x00200000
#define FMOD_MEMORY_SECONDARY 0x00400000
#define FMOD_MEMORY_ALL 0xFFFFFFFF

#define FMOD_PRESET_OFF \
    { 0, -1, 7.5f, 1.00f, -10000, -10000, 0, 1.00f, 1.00f, 1.0f, -2602, 0.007f, { 0.0f, 0.0f, 0.0f }, 200, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 0.0f, 0.0f, 0x33f }
#define FMOD_PRESET_GENERIC \
    { 0, 0, 7.5f, 1.00f, -1000, -100, 0, 1.49f, 0.83f, 1.0f, -2602, 0.007f, { 0.0f, 0.0f, 0.0f }, 200, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_PADDEDCELL \
    { 0, 1, 1.4f, 1.00f, -1000, -6000, 0, 0.17f, 0.10f, 1.0f, -1204, 0.001f, { 0.0f, 0.0f, 0.0f }, 207, 0.002f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_ROOM \
    { 0, 2, 1.9f, 1.00f, -1000, -454, 0, 0.40f, 0.83f, 1.0f, -1646, 0.002f, { 0.0f, 0.0f, 0.0f }, 53, 0.003f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_BATHROOM \
    { 0, 3, 1.4f, 1.00f, -1000, -1200, 0, 1.49f, 0.54f, 1.0f, -370, 0.007f, { 0.0f, 0.0f, 0.0f }, 1030, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 60.0f, 0x3f }
#define FMOD_PRESET_LIVINGROOM \
    { 0, 4, 2.5f, 1.00f, -1000, -6000, 0, 0.50f, 0.10f, 1.0f, -1376, 0.003f, { 0.0f, 0.0f, 0.0f }, -1104, 0.004f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_STONEROOM \
    { 0, 5, 11.6f, 1.00f, -1000, -300, 0, 2.31f, 0.64f, 1.0f, -711, 0.012f, { 0.0f, 0.0f, 0.0f }, 83, 0.017f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_AUDITORIUM \
    { 0, 6, 21.6f, 1.00f, -1000, -476, 0, 4.32f, 0.59f, 1.0f, -789, 0.020f, { 0.0f, 0.0f, 0.0f }, -289, 0.030f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_CONCERTHALL \
    { 0, 7, 19.6f, 1.00f, -1000, -500, 0, 3.92f, 0.70f, 1.0f, -1230, 0.020f, { 0.0f, 0.0f, 0.0f }, -2, 0.029f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_CAVE \
    { 0, 8, 14.6f, 1.00f, -1000, 0, 0, 2.91f, 1.30f, 1.0f, -602, 0.015f, { 0.0f, 0.0f, 0.0f }, -302, 0.022f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x1f }
#define FMOD_PRESET_ARENA \
    { 0, 9, 36.2f, 1.00f, -1000, -698, 0, 7.24f, 0.33f, 1.0f, -1166, 0.020f, { 0.0f, 0.0f, 0.0f }, 16, 0.030f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_HANGAR \
    { 0, 10, 50.3f, 1.00f, -1000, -1000, 0, 10.05f, 0.23f, 1.0f, -602, 0.020f, { 0.0f, 0.0f, 0.0f }, 198, 0.030f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_CARPETTEDHALLWAY \
    { 0, 11, 1.9f, 1.00f, -1000, -4000, 0, 0.30f, 0.10f, 1.0f, -1831, 0.002f, { 0.0f, 0.0f, 0.0f }, -1630, 0.030f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_HALLWAY \
    { 0, 12, 1.8f, 1.00f, -1000, -300, 0, 1.49f, 0.59f, 1.0f, -1219, 0.007f, { 0.0f, 0.0f, 0.0f }, 441, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_STONECORRIDOR \
    { 0, 13, 13.5f, 1.00f, -1000, -237, 0, 2.70f, 0.79f, 1.0f, -1214, 0.013f, { 0.0f, 0.0f, 0.0f }, 395, 0.020f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_ALLEY \
    { 0, 14, 7.5f, 0.30f, -1000, -270, 0, 1.49f, 0.86f, 1.0f, -1204, 0.007f, { 0.0f, 0.0f, 0.0f }, -4, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.125f, 0.95f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_FOREST \
    { 0, 15, 38.0f, 0.30f, -1000, -3300, 0, 1.49f, 0.54f, 1.0f, -2560, 0.162f, { 0.0f, 0.0f, 0.0f }, -229, 0.088f, { 0.0f, 0.0f, 0.0f }, 0.125f, 1.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 79.0f, 100.0f, 0x3f }
#define FMOD_PRESET_CITY \
    { 0, 16, 7.5f, 0.50f, -1000, -800, 0, 1.49f, 0.67f, 1.0f, -2273, 0.007f, { 0.0f, 0.0f, 0.0f }, -1691, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 50.0f, 100.0f, 0x3f }
#define FMOD_PRESET_MOUNTAINS \
    { 0, 17, 100.0f, 0.27f, -1000, -2500, 0, 1.49f, 0.21f, 1.0f, -2780, 0.300f, { 0.0f, 0.0f, 0.0f }, -1434, 0.100f, { 0.0f, 0.0f, 0.0f }, 0.250f, 1.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 27.0f, 100.0f, 0x1f }
#define FMOD_PRESET_QUARRY \
    { 0, 18, 17.5f, 1.00f, -1000, -1000, 0, 1.49f, 0.83f, 1.0f, -10000, 0.061f, { 0.0f, 0.0f, 0.0f }, 500, 0.025f, { 0.0f, 0.0f, 0.0f }, 0.125f, 0.70f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }
#define FMOD_PRESET_PLAIN \
    { 0, 19, 42.5f, 0.21f, -1000, -2000, 0, 1.49f, 0.50f, 1.0f, -2466, 0.179f, { 0.0f, 0.0f, 0.0f }, -1926, 0.100f, { 0.0f, 0.0f, 0.0f }, 0.250f, 1.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 21.0f, 100.0f, 0x3f }
#define FMOD_PRESET_PARKINGLOT \
    { 0, 20, 8.3f, 1.00f, -1000, 0, 0, 1.65f, 1.50f, 1.0f, -1363, 0.008f, { 0.0f, 0.0f, 0.0f }, -1153, 0.012f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x1f }
#define FMOD_PRESET_SEWERPIPE \
    { 0, 21, 1.7f, 0.80f, -1000, -1000, 0, 2.81f, 0.14f, 1.0f, 429, 0.014f, { 0.0f, 0.0f, 0.0f }, 1023, 0.021f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 0.25f, 0.000f, -5.0f, 5000.0f, 250.0f, 0.0f, 80.0f, 60.0f, 0x3f }
#define FMOD_PRESET_UNDERWATER \
    { 0, 22, 1.8f, 1.00f, -1000, -4000, 0, 1.49f, 0.10f, 1.0f, -449, 0.007f, { 0.0f, 0.0f, 0.0f }, 1700, 0.011f, { 0.0f, 0.0f, 0.0f }, 0.250f, 0.00f, 1.18f, 0.348f, -5.0f, 5000.0f, 250.0f, 0.0f, 100.0f, 100.0f, 0x3f }

// TODO: proper calling conventions

#if defined(XMODEX_SYSTEM_WIN)

#define F_STDCALL __stdcall
#define F_CDECL __cdecl
#define F_DLLEXPORT __declspec(dllexport)

#define F_API __declspec(dllexport) F_STDCALL
#define F_CALLBACK F_STDCALL

#else

#define F_STDCALL
#define F_CDECL
#define F_DLLEXPORT

#define F_API F_STDCALL
#define F_CALLBACK F_STDCALL

#endif

typedef int32_t FMOD_BOOL;
typedef uint32_t FMOD_MODE;
typedef uint32_t FMOD_TIMEUNIT;
typedef uint32_t FMOD_INITFLAGS;
typedef uint32_t FMOD_CAPS;
typedef uint32_t FMOD_DEBUGLEVEL;
typedef uint32_t FMOD_MEMORY_TYPE;

struct FMOD_SYSTEM;
struct FMOD_SOUND;
struct FMOD_CHANNEL;
struct FMOD_CHANNELGROUP;
struct FMOD_DSP;
struct FMOD_POLYGON;
struct FMOD_GEOMETRY;
struct FMOD_SYNCPOINT;
struct FMOD_SOUNDGROUP;

struct FMOD_CODEC_STATE;
struct FMOD_CODEC_WAVEFORMAT;
struct FMOD_CREATESOUNDEXINFO;

struct FMOD_DSP_STATE;

struct FMOD_OUTPUT_STATE;

enum FMOD_RESULT {
    FMOD_OK,
    FMOD_ERR_ALREADYLOCKED,
    FMOD_ERR_BADCOMMAND,
    FMOD_ERR_CDDA_DRIVERS,
    FMOD_ERR_CDDA_INIT,
    FMOD_ERR_CDDA_INVALID_DEVICE,
    FMOD_ERR_CDDA_NOAUDIO,
    FMOD_ERR_CDDA_NODEVICES,
    FMOD_ERR_CDDA_NODISC,
    FMOD_ERR_CDDA_READ,
    FMOD_ERR_CHANNEL_ALLOC,
    FMOD_ERR_CHANNEL_STOLEN,
    FMOD_ERR_COM,
    FMOD_ERR_DMA,
    FMOD_ERR_DSP_CONNECTION,
    FMOD_ERR_DSP_FORMAT,
    FMOD_ERR_DSP_NOTFOUND,
    FMOD_ERR_DSP_RUNNING,
    FMOD_ERR_DSP_TOOMANYCONNECTIONS,
    FMOD_ERR_FILE_BAD,
    FMOD_ERR_FILE_COULDNOTSEEK,
    FMOD_ERR_FILE_DISKEJECTED,
    FMOD_ERR_FILE_EOF,
    FMOD_ERR_FILE_NOTFOUND,
    FMOD_ERR_FILE_UNWANTED,
    FMOD_ERR_FORMAT,
    FMOD_ERR_HTTP,
    FMOD_ERR_HTTP_ACCESS,
    FMOD_ERR_HTTP_PROXY_AUTH,
    FMOD_ERR_HTTP_SERVER_ERROR,
    FMOD_ERR_HTTP_TIMEOUT,
    FMOD_ERR_INITIALIZATION,
    FMOD_ERR_INITIALIZED,
    FMOD_ERR_INTERNAL,
    FMOD_ERR_INVALID_ADDRESS,
    FMOD_ERR_INVALID_FLOAT,
    FMOD_ERR_INVALID_HANDLE,
    FMOD_ERR_INVALID_PARAM,
    FMOD_ERR_INVALID_SPEAKER,
    FMOD_ERR_INVALID_VECTOR,
    FMOD_ERR_IRX,
    FMOD_ERR_MAXAUDIBLE,
    FMOD_ERR_MEMORY,
    FMOD_ERR_MEMORY_IOP,
    FMOD_ERR_MEMORY_SRAM,
    FMOD_ERR_MEMORY_CANTPOINT,
    FMOD_ERR_NEEDS2D,
    FMOD_ERR_NEEDS3D,
    FMOD_ERR_NEEDSHARDWARE,
    FMOD_ERR_NEEDSSOFTWARE,
    FMOD_ERR_NET_CONNECT,
    FMOD_ERR_NET_SOCKET_ERROR,
    FMOD_ERR_NET_URL,
    FMOD_ERR_NET_WOULD_BLOCK,
    FMOD_ERR_NOTREADY,
    FMOD_ERR_OUTPUT_ALLOCATED,
    FMOD_ERR_OUTPUT_CREATEBUFFER,
    FMOD_ERR_OUTPUT_DRIVERCALL,
    FMOD_ERR_OUTPUT_FORMAT,
    FMOD_ERR_OUTPUT_INIT,
    FMOD_ERR_OUTPUT_NOHARDWARE,
    FMOD_ERR_OUTPUT_NOSOFTWARE,
    FMOD_ERR_PAN,
    FMOD_ERR_PLUGIN,
    FMOD_ERR_PLUGIN_MISSING,
    FMOD_ERR_PLUGIN_RESOURCE,
    FMOD_ERR_PLUGIN_INSTANCES,
    FMOD_ERR_RECORD,
    FMOD_ERR_REVERB_INSTANCE,
    FMOD_ERR_SUBSOUNDS,
    FMOD_ERR_SUBSOUND_ALLOCATED,
    FMOD_ERR_TAGNOTFOUND,
    FMOD_ERR_TOOMANYCHANNELS,
    FMOD_ERR_UNIMPLEMENTED,
    FMOD_ERR_UNINITIALIZED,
    FMOD_ERR_UNSUPPORTED,
    FMOD_ERR_UPDATE,
    FMOD_ERR_VERSION,
    FMOD_ERR_EVENT_FAILED,
    FMOD_ERR_EVENT_INTERNAL,
    FMOD_ERR_EVENT_INFOONLY,
    FMOD_ERR_EVENT_MAXSTREAMS,
    FMOD_ERR_EVENT_MISMATCH,
    FMOD_ERR_EVENT_NAMECONFLICT,
    FMOD_ERR_EVENT_NOTFOUND,
    FMOD_RESULT_FORCEINT = 65536
};

enum FMOD_OUTPUTTYPE {
    FMOD_OUTPUTTYPE_AUTODETECT,
    FMOD_OUTPUTTYPE_UNKNOWN,
    FMOD_OUTPUTTYPE_NOSOUND,
    FMOD_OUTPUTTYPE_WAVWRITER,
    FMOD_OUTPUTTYPE_NOSOUND_NRT,
    FMOD_OUTPUTTYPE_WAVWRITER_NRT,
    FMOD_OUTPUTTYPE_DSOUND,
    FMOD_OUTPUTTYPE_WINMM,
    FMOD_OUTPUTTYPE_OPENAL,
    FMOD_OUTPUTTYPE_WASAPI,
    FMOD_OUTPUTTYPE_ASIO,
    FMOD_OUTPUTTYPE_OSS,
    FMOD_OUTPUTTYPE_ALSA,
    FMOD_OUTPUTTYPE_ESD,
    FMOD_OUTPUTTYPE_SOUNDMANAGER,
    FMOD_OUTPUTTYPE_COREAUDIO,
    FMOD_OUTPUTTYPE_XBOX,
    FMOD_OUTPUTTYPE_PS2,
    FMOD_OUTPUTTYPE_PS3,
    FMOD_OUTPUTTYPE_GC,
    FMOD_OUTPUTTYPE_XBOX360,
    FMOD_OUTPUTTYPE_PSP,
    FMOD_OUTPUTTYPE_WII,
    FMOD_OUTPUTTYPE_MAX,
    FMOD_OUTPUTTYPE_FORCEINT = 65536
};

enum FMOD_SPEAKERMODE {
    FMOD_SPEAKERMODE_RAW,
    FMOD_SPEAKERMODE_MONO,
    FMOD_SPEAKERMODE_STEREO,
    FMOD_SPEAKERMODE_QUAD,
    FMOD_SPEAKERMODE_SURROUND,
    FMOD_SPEAKERMODE_5POINT1,
    FMOD_SPEAKERMODE_7POINT1,
    FMOD_SPEAKERMODE_PROLOGIC,
    FMOD_SPEAKERMODE_MAX,
    FMOD_SPEAKERMODE_FORCEINT = 65536
};

enum FMOD_SPEAKER {
    FMOD_SPEAKER_FRONT_LEFT,
    FMOD_SPEAKER_FRONT_RIGHT,
    FMOD_SPEAKER_FRONT_CENTER,
    FMOD_SPEAKER_LOW_FREQUENCY,
    FMOD_SPEAKER_BACK_LEFT,
    FMOD_SPEAKER_BACK_RIGHT,
    FMOD_SPEAKER_SIDE_LEFT,
    FMOD_SPEAKER_SIDE_RIGHT,
    FMOD_SPEAKER_MAX,
    FMOD_SPEAKER_MONO = FMOD_SPEAKER_FRONT_LEFT,
    FMOD_SPEAKER_NULL = FMOD_SPEAKER_MAX,
    FMOD_SPEAKER_SBL = FMOD_SPEAKER_SIDE_LEFT,
    FMOD_SPEAKER_SBR = FMOD_SPEAKER_SIDE_RIGHT,
    FMOD_SPEAKER_FORCEINT = 65536
};

enum FMOD_PLUGINTYPE {
    FMOD_PLUGINTYPE_OUTPUT,
    FMOD_PLUGINTYPE_CODEC,
    FMOD_PLUGINTYPE_DSP,
    FMOD_PLUGINTYPE_MAX,
    FMOD_PLUGINTYPE_FORCEINT = 65536
};

enum FMOD_SOUND_TYPE {
    FMOD_SOUND_TYPE_UNKNOWN,
    FMOD_SOUND_TYPE_AAC,
    FMOD_SOUND_TYPE_AIFF,
    FMOD_SOUND_TYPE_ASF,
    FMOD_SOUND_TYPE_AT3,
    FMOD_SOUND_TYPE_CDDA,
    FMOD_SOUND_TYPE_DLS,
    FMOD_SOUND_TYPE_FLAC,
    FMOD_SOUND_TYPE_FSB,
    FMOD_SOUND_TYPE_GCADPCM,
    FMOD_SOUND_TYPE_IT,
    FMOD_SOUND_TYPE_MIDI,
    FMOD_SOUND_TYPE_MOD,
    FMOD_SOUND_TYPE_MPEG,
    FMOD_SOUND_TYPE_OGGVORBIS,
    FMOD_SOUND_TYPE_PLAYLIST,
    FMOD_SOUND_TYPE_RAW,
    FMOD_SOUND_TYPE_S3M,
    FMOD_SOUND_TYPE_SF2,
    FMOD_SOUND_TYPE_USER,
    FMOD_SOUND_TYPE_WAV,
    FMOD_SOUND_TYPE_XM,
    FMOD_SOUND_TYPE_XMA,
    FMOD_SOUND_TYPE_VAG,
    FMOD_SOUND_TYPE_MAX,
    FMOD_SOUND_TYPE_FORCEINT = 65536
};

enum FMOD_SOUND_FORMAT {
    FMOD_SOUND_FORMAT_NONE,
    FMOD_SOUND_FORMAT_PCM8,
    FMOD_SOUND_FORMAT_PCM16,
    FMOD_SOUND_FORMAT_PCM24,
    FMOD_SOUND_FORMAT_PCM32,
    FMOD_SOUND_FORMAT_PCMFLOAT,
    FMOD_SOUND_FORMAT_GCADPCM,
    FMOD_SOUND_FORMAT_IMAADPCM,
    FMOD_SOUND_FORMAT_VAG,
    FMOD_SOUND_FORMAT_XMA,
    FMOD_SOUND_FORMAT_MPEG,
    FMOD_SOUND_FORMAT_MAX,
    FMOD_SOUND_FORMAT_FORCEINT = 65536
};

enum FMOD_OPENSTATE {
    FMOD_OPENSTATE_READY = 0,
    FMOD_OPENSTATE_LOADING,
    FMOD_OPENSTATE_ERROR,
    FMOD_OPENSTATE_CONNECTING,
    FMOD_OPENSTATE_BUFFERING,
    FMOD_OPENSTATE_SEEKING,
    FMOD_OPENSTATE_MAX,
    FMOD_OPENSTATE_FORCEINT = 65536
};

enum FMOD_CHANNEL_CALLBACKTYPE {
    FMOD_CHANNEL_CALLBACKTYPE_END,
    FMOD_CHANNEL_CALLBACKTYPE_VIRTUALVOICE,
    FMOD_CHANNEL_CALLBACKTYPE_SYNCPOINT,
    FMOD_CHANNEL_CALLBACKTYPE_MAX,
    FMOD_CHANNEL_CALLBACKTYPE_FORCEINT = 65536
};

enum FMOD_SYSTEM_CALLBACKTYPE {
    FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED,
    FMOD_SYSTEM_CALLBACKTYPE_MAX = 32,
    FMOD_SYSTEM_CALLBACKTYPE_FORCEINT = 65536
};

enum FMOD_DSP_FFT_WINDOW {
    FMOD_DSP_FFT_WINDOW_RECT,
    FMOD_DSP_FFT_WINDOW_TRIANGLE,
    FMOD_DSP_FFT_WINDOW_HAMMING,
    FMOD_DSP_FFT_WINDOW_HANNING,
    FMOD_DSP_FFT_WINDOW_BLACKMAN,
    FMOD_DSP_FFT_WINDOW_BLACKMANHARRIS,
    FMOD_DSP_FFT_WINDOW_MAX,
    FMOD_DSP_FFT_WINDOW_FORCEINT = 65536
};

enum FMOD_DSP_RESAMPLER {
    FMOD_DSP_RESAMPLER_NOINTERP,
    FMOD_DSP_RESAMPLER_LINEAR,
    FMOD_DSP_RESAMPLER_CUBIC,
    FMOD_DSP_RESAMPLER_SPLINE,
    FMOD_DSP_RESAMPLER_MAX,
    FMOD_DSP_RESAMPLER_FORCEINT = 65536
};

enum FMOD_TAGTYPE {
    FMOD_TAGTYPE_UNKNOWN = 0,
    FMOD_TAGTYPE_ID3V1,
    FMOD_TAGTYPE_ID3V2,
    FMOD_TAGTYPE_VORBISCOMMENT,
    FMOD_TAGTYPE_SHOUTCAST,
    FMOD_TAGTYPE_ICECAST,
    FMOD_TAGTYPE_ASF,
    FMOD_TAGTYPE_MIDI,
    FMOD_TAGTYPE_PLAYLIST,
    FMOD_TAGTYPE_FMOD,
    FMOD_TAGTYPE_USER,
    FMOD_TAGTYPE_MAX,
    FMOD_TAGTYPE_FORCEINT = 65536
};

enum FMOD_TAGDATATYPE {
    FMOD_TAGDATATYPE_BINARY = 0,
    FMOD_TAGDATATYPE_INT,
    FMOD_TAGDATATYPE_FLOAT,
    FMOD_TAGDATATYPE_STRING,
    FMOD_TAGDATATYPE_STRING_UTF16,
    FMOD_TAGDATATYPE_STRING_UTF16BE,
    FMOD_TAGDATATYPE_STRING_UTF8,
    FMOD_TAGDATATYPE_CDTOC,
    FMOD_TAGDATATYPE_MAX,
    FMOD_TAGDATATYPE_FORCEINT = 65536
};

enum FMOD_DELAYTYPE {
    FMOD_DELAYTYPE_END_MS,
    FMOD_DELAYTYPE_DSPCLOCK_START,
    FMOD_DELAYTYPE_DSPCLOCK_END,
    FMOD_DELAYTYPE_DSPCLOCK_PAUSE,
    FMOD_DELAYTYPE_MAX,
    FMOD_DELAYTYPE_FORCEINT = 65536
};

enum FMOD_SPEAKERMAPTYPE {
    FMOD_SPEAKERMAPTYPE_DEFAULT,
    FMOD_SPEAKERMAPTYPE_ALLMONO,
    FMOD_SPEAKERMAPTYPE_ALLSTEREO
};

enum FMOD_CHANNELINDEX {
    FMOD_CHANNEL_FREE = -1,
    FMOD_CHANNEL_REUSE = -2
};

enum FMOD_DSP_TYPE {
    FMOD_DSP_TYPE_UNKNOWN,
    FMOD_DSP_TYPE_MIXER,
    FMOD_DSP_TYPE_OSCILLATOR,
    FMOD_DSP_TYPE_LOWPASS,
    FMOD_DSP_TYPE_ITLOWPASS,
    FMOD_DSP_TYPE_HIGHPASS,
    FMOD_DSP_TYPE_ECHO,
    FMOD_DSP_TYPE_FLANGE,
    FMOD_DSP_TYPE_DISTORTION,
    FMOD_DSP_TYPE_NORMALIZE,
    FMOD_DSP_TYPE_PARAMEQ,
    FMOD_DSP_TYPE_PITCHSHIFT,
    FMOD_DSP_TYPE_CHORUS,
    FMOD_DSP_TYPE_REVERB,
    FMOD_DSP_TYPE_VSTPLUGIN,
    FMOD_DSP_TYPE_WINAMPPLUGIN,
    FMOD_DSP_TYPE_ITECHO,
    FMOD_DSP_TYPE_COMPRESSOR,
    FMOD_DSP_TYPE_SFXREVERB,
    FMOD_DSP_TYPE_LOWPASS_SIMPLE,
    FMOD_DSP_TYPE_FORCEINT = 65536
};

enum FMOD_DSP_OSCILLATOR {
    FMOD_DSP_OSCILLATOR_TYPE,
    FMOD_DSP_OSCILLATOR_RATE
};

enum FMOD_DSP_LOWPASS {
    FMOD_DSP_LOWPASS_CUTOFF,
    FMOD_DSP_LOWPASS_RESONANCE
};

enum FMOD_DSP_ITLOWPASS {
    FMOD_DSP_ITLOWPASS_CUTOFF,
    FMOD_DSP_ITLOWPASS_RESONANCE
};

enum FMOD_DSP_HIGHPASS {
    FMOD_DSP_HIGHPASS_CUTOFF,
    FMOD_DSP_HIGHPASS_RESONANCE
};

enum FMOD_DSP_ECHO {
    FMOD_DSP_ECHO_DELAY,
    FMOD_DSP_ECHO_DECAYRATIO,
    FMOD_DSP_ECHO_MAXCHANNELS,
    FMOD_DSP_ECHO_DRYMIX,
    FMOD_DSP_ECHO_WETMIX
};

enum FMOD_DSP_FLANGE {
    FMOD_DSP_FLANGE_DRYMIX,
    FMOD_DSP_FLANGE_WETMIX,
    FMOD_DSP_FLANGE_DEPTH,
    FMOD_DSP_FLANGE_RATE
};

enum FMOD_DSP_DISTORTION {
    FMOD_DSP_DISTORTION_LEVEL
};

enum FMOD_DSP_NORMALIZE {
    FMOD_DSP_NORMALIZE_FADETIME,
    FMOD_DSP_NORMALIZE_THRESHHOLD,
    FMOD_DSP_NORMALIZE_MAXAMP
};

enum FMOD_DSP_PARAMEQ {
    FMOD_DSP_PARAMEQ_CENTER,
    FMOD_DSP_PARAMEQ_BANDWIDTH,
    FMOD_DSP_PARAMEQ_GAIN
};

enum FMOD_DSP_PITCHSHIFT {
    FMOD_DSP_PITCHSHIFT_PITCH,
    FMOD_DSP_PITCHSHIFT_FFTSIZE,
    FMOD_DSP_PITCHSHIFT_OVERLAP,
    FMOD_DSP_PITCHSHIFT_MAXCHANNELS
};

enum FMOD_DSP_CHORUS {
    FMOD_DSP_CHORUS_DRYMIX,
    FMOD_DSP_CHORUS_WETMIX1,
    FMOD_DSP_CHORUS_WETMIX2,
    FMOD_DSP_CHORUS_WETMIX3,
    FMOD_DSP_CHORUS_DELAY,
    FMOD_DSP_CHORUS_RATE,
    FMOD_DSP_CHORUS_DEPTH,
    FMOD_DSP_CHORUS_FEEDBACK
};

enum FMOD_DSP_REVERB {
    FMOD_DSP_REVERB_ROOMSIZE,
    FMOD_DSP_REVERB_DAMP,
    FMOD_DSP_REVERB_WETMIX,
    FMOD_DSP_REVERB_DRYMIX,
    FMOD_DSP_REVERB_WIDTH,
    FMOD_DSP_REVERB_MODE
};

enum FMOD_DSP_ITECHO {
    FMOD_DSP_ITECHO_WETDRYMIX,
    FMOD_DSP_ITECHO_FEEDBACK,
    FMOD_DSP_ITECHO_LEFTDELAY,
    FMOD_DSP_ITECHO_RIGHTDELAY,
    FMOD_DSP_ITECHO_PANDELAY
};

enum FMOD_DSP_COMPRESSOR {
    FMOD_DSP_COMPRESSOR_THRESHOLD,
    FMOD_DSP_COMPRESSOR_ATTACK,
    FMOD_DSP_COMPRESSOR_RELEASE,
    FMOD_DSP_COMPRESSOR_GAINMAKEUP
};

enum FMOD_DSP_SFXREVERB {
    FMOD_DSP_SFXREVERB_DRYLEVEL,
    FMOD_DSP_SFXREVERB_ROOM,
    FMOD_DSP_SFXREVERB_ROOMHF,
    FMOD_DSP_SFXREVERB_ROOMROLLOFFFACTOR,
    FMOD_DSP_SFXREVERB_DECAYTIME,
    FMOD_DSP_SFXREVERB_DECAYHFRATIO,
    FMOD_DSP_SFXREVERB_REFLECTIONSLEVEL,
    FMOD_DSP_SFXREVERB_REFLECTIONSDELAY,
    FMOD_DSP_SFXREVERB_REVERBLEVEL,
    FMOD_DSP_SFXREVERB_REVERBDELAY,
    FMOD_DSP_SFXREVERB_DIFFUSION,
    FMOD_DSP_SFXREVERB_DENSITY,
    FMOD_DSP_SFXREVERB_HFREFERENCE,
    FMOD_DSP_SFXREVERB_ROOMLF,
    FMOD_DSP_SFXREVERB_LFREFERENCE
};

enum FMOD_DSP_LOWPASS_SIMPLE {
    FMOD_DSP_LOWPASS_SIMPLE_CUTOFF
};

enum FMOD_SOUNDGROUP_BEHAVIOR {
    FMOD_SOUNDGROUP_BEHAVIOR_FAIL,
    FMOD_SOUNDGROUP_BEHAVIOR_MUTE,
    FMOD_SOUNDGROUP_BEHAVIOR_STEALLOWEST,
    FMOD_SOUNDGROUP_BEHAVIOR_MAX,
    FMOD_SOUNDGROUP_BEHAVIOR_FORCEINT = 65536
};

typedef FMOD_RESULT(F_CALLBACK* FMOD_CHANNEL_CALLBACK)(
    FMOD_CHANNEL* channel,
    FMOD_CHANNEL_CALLBACKTYPE type,
    int32_t command,
    uint32_t commanddata1,
    uint32_t commanddata2);

typedef FMOD_RESULT(F_CALLBACK* FMOD_FILE_OPENCALLBACK)(
    const char* name,
    int32_t unicode,
    uint32_t* filesize,
    void** handle,
    void** userdata);

typedef FMOD_RESULT(F_CALLBACK* FMOD_FILE_CLOSECALLBACK)(
    void* handle,
    void* userdata);

typedef FMOD_RESULT(F_CALLBACK* FMOD_FILE_READCALLBACK)(
    void* handle,
    void* buffer,
    uint32_t sizebytes,
    uint32_t* bytesread,
    void* userdata);

typedef FMOD_RESULT(F_CALLBACK* FMOD_FILE_SEEKCALLBACK)(
    void* handle,
    uint32_t pos,
    void* userdata);

typedef void*(F_CALLBACK* FMOD_MEMORY_ALLOCCALLBACK)(
    int32_t size,
    FMOD_MEMORY_TYPE type);

typedef void*(F_CALLBACK* FMOD_MEMORY_REALLOCCALLBACK)(
    void* ptr,
    int32_t size,
    FMOD_MEMORY_TYPE type);

typedef void(F_CALLBACK* FMOD_MEMORY_FREECALLBACK)(
    void* ptr,
    FMOD_MEMORY_TYPE);

typedef FMOD_RESULT(F_CALLBACK* FMOD_SOUND_NONBLOCKCALLBACK)(
    FMOD_SOUND* sound,
    FMOD_RESULT result);

typedef FMOD_RESULT(F_CALLBACK* FMOD_SOUND_PCMREADCALLBACK)(
    FMOD_SOUND* sound,
    void* data,
    uint32_t datalen);

typedef FMOD_RESULT(F_CALLBACK* FMOD_SOUND_PCMSETPOSCALLBACK)(
    FMOD_SOUND* sound,
    int32_t subsound,
    uint32_t position,
    FMOD_TIMEUNIT postype);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_OPENCALLBACK)(
    FMOD_CODEC_STATE* codec_state,
    FMOD_MODE usermode,
    FMOD_CREATESOUNDEXINFO* userexinfo);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_CLOSECALLBACK)(
    FMOD_CODEC_STATE* codec_state);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_READCALLBACK)(
    FMOD_CODEC_STATE* codec_state,
    void* buffer,
    uint32_t sizebytes,
    uint32_t* bytesread);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_GETLENGTHCALLBACK)(
    FMOD_CODEC_STATE* codec_state,
    uint32_t* length,
    FMOD_TIMEUNIT lengthtype);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_SETPOSITIONCALLBACK)(
    FMOD_CODEC_STATE* codec_state,
    int32_t subsound,
    uint32_t position,
    FMOD_TIMEUNIT postype);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_GETPOSITIONCALLBACK)(
    FMOD_CODEC_STATE* codec_state,
    uint32_t* position,
    FMOD_TIMEUNIT postype);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_SOUNDCREATECALLBACK)(
    FMOD_CODEC_STATE* codec_state,
    int32_t subsound,
    FMOD_SOUND* sound);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_METADATACALLBACK)(
    FMOD_CODEC_STATE* codec_state,
    FMOD_TAGTYPE tagtype,
    char* name,
    void* data,
    uint32_t datalen,
    FMOD_TAGDATATYPE datatype,
    int32_t unique);

typedef FMOD_RESULT(F_CALLBACK* FMOD_CODEC_GETWAVEFORMAT)(
    FMOD_CODEC_STATE* codec_state,
    int32_t index,
    FMOD_CODEC_WAVEFORMAT* waveformat);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_CREATECALLBACK)(
    FMOD_DSP_STATE* dsp_state);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_RELEASECALLBACK)(
    FMOD_DSP_STATE* dsp_state);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_RESETCALLBACK)(
    FMOD_DSP_STATE* dsp_state);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_READCALLBACK)(
    FMOD_DSP_STATE* dsp_state,
    float* inbuffer,
    float* outbuffer,
    uint32_t length,
    int32_t inchannels,
    int32_t outchannels);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_SETPOSITIONCALLBACK)(
    FMOD_DSP_STATE* dsp_state,
    uint32_t pos);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_SETPARAMCALLBACK)(
    FMOD_DSP_STATE* dsp_state,
    int32_t index,
    float value);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_GETPARAMCALLBACK)(
    FMOD_DSP_STATE* dsp_state,
    int32_t index,
    float* value,
    char* valuestr);

typedef FMOD_RESULT(F_CALLBACK* FMOD_DSP_DIALOGCALLBACK)(
    FMOD_DSP_STATE* dsp_state,
    void* hwnd,
    int32_t show);

typedef FMOD_RESULT(F_CALLBACK* FMOD_SYSTEM_CALLBACK)(
    FMOD_SYSTEM* system,
    FMOD_SYSTEM_CALLBACKTYPE type,
    uint32_t commandata1,
    uint32_t commanddata2);

typedef float(F_CALLBACK* FMOD_3D_ROLLOFFCALLBACK)(FMOD_CHANNEL* channel, float distance);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETNUMDRIVERSCALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    int32_t* numdrivers);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETDRIVERNAMECALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    int32_t id,
    char* name,
    int32_t namelen);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETDRIVERCAPSCALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    int32_t id,
    FMOD_CAPS* caps);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_INITCALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    int32_t selecteddriver,
    FMOD_INITFLAGS flags,
    int32_t* outputrate,
    int32_t outputchannels,
    FMOD_SOUND_FORMAT* outputformat,
    int32_t dspbufferlength,
    int32_t dspnumbuffers,
    void* extradriverdata);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_CLOSECALLBACK)(FMOD_OUTPUT_STATE* output_state);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_UPDATECALLBACK)(FMOD_OUTPUT_STATE* output_state);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETHANDLECALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    void** handle);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_GETPOSITIONCALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    uint32_t* pcm);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_LOCKCALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    uint32_t offset,
    uint32_t length,
    void** ptr1,
    void** ptr2,
    uint32_t* len1,
    uint32_t* len2);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_UNLOCKCALLBACK)(
    FMOD_OUTPUT_STATE* output_state,
    void* ptr1,
    void* ptr2,
    uint32_t len1,
    uint32_t len2);

typedef FMOD_RESULT(F_CALLBACK* FMOD_OUTPUT_READFROMMIXER)(
    FMOD_OUTPUT_STATE* output_state,
    void* buffer,
    uint32_t length);

struct FMOD_VECTOR {
        float x;
        float y;
        float z;
};

struct FMOD_COMPLEX {
        float re;
        float im;
};

struct FMOD_GUID {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t Data4[8];
};

struct FMOD_TAG {
        FMOD_TAGTYPE type;
        FMOD_TAGDATATYPE datatype;
        char* name;
        void* data;
        uint32_t datalen;
        FMOD_BOOL updated;
};

struct FMOD_CREATESOUNDEXINFO {
        int32_t cbsize;
        uint32_t length;
        uint32_t fileoffset;
        int32_t numchannels;
        int32_t defaultfrequency;
        FMOD_SOUND_FORMAT format;
        uint32_t decodebuffersize;
        int32_t initialsubsound;
        int32_t numsubsounds;
        int32_t* inclusionlist;
        int32_t inclusionlistnum;
        FMOD_SOUND_PCMREADCALLBACK pcmreadcallback;
        FMOD_SOUND_PCMSETPOSCALLBACK pcmsetposcallback;
        FMOD_SOUND_NONBLOCKCALLBACK nonblockcallback;
        const char* dlsname;
        const char* encryptionkey;
        int32_t maxpolyphony;
        void* userdata;
        FMOD_SOUND_TYPE suggestedsoundtype;
        FMOD_FILE_OPENCALLBACK useropen;
        FMOD_FILE_CLOSECALLBACK userclose;
        FMOD_FILE_READCALLBACK userread;
        FMOD_FILE_SEEKCALLBACK userseek;
        FMOD_SPEAKERMAPTYPE speakermap;
        FMOD_SOUNDGROUP* initialsoundgroup;
        uint32_t initialseekposition;
        FMOD_TIMEUNIT initialseekpostype;
};

struct FMOD_REVERB_PROPERTIES {
        int32_t Instance;
        int32_t Environment;
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

struct FMOD_REVERB_CHANNELPROPERTIES {
        int32_t Direct;
        int32_t DirectHF;
        int32_t Room;
        int32_t RoomHF;
        int32_t Obstruction;
        float ObstructionLFRatio;
        int32_t Occlusion;
        float OcclusionLFRatio;
        float OcclusionRoomRatio;
        float OcclusionDirectRatio;
        int32_t Exclusion;
        float ExclusionLFRatio;
        int32_t OutsideVolumeHF;
        float DopplerFactor;
        float RolloffFactor;
        float RoomRolloffFactor;
        float AirAbsorptionFactor;
        uint32_t Flags;
};

struct FMOD_ADVANCEDSETTINGS {
        int32_t cbsize;
        int32_t maxMPEGcodecs;
        int32_t maxADPCMcodecs;
        int32_t maxXMAcodecs;
        int32_t maxPCMcodecs;
        int32_t ASIONumChannels;
        char** ASIOChannelList;
        FMOD_SPEAKER* ASIOSpeakerList;
        int32_t max3DReverbDSPs;
        float HRTFMinAngle;
        float HRTFMaxAngle;
        float HRTFFreq;
        float vol0virtualvol;
        int32_t eventqueuesize;
};

struct FMOD_CODEC_DESCRIPTION {
        const char* name;
        uint32_t version;
        int32_t defaultasstream;
        FMOD_TIMEUNIT timeunits;
        FMOD_CODEC_OPENCALLBACK open;
        FMOD_CODEC_CLOSECALLBACK close;
        FMOD_CODEC_READCALLBACK read;
        FMOD_CODEC_GETLENGTHCALLBACK getlength;
        FMOD_CODEC_SETPOSITIONCALLBACK setposition;
        FMOD_CODEC_GETPOSITIONCALLBACK getposition;
        FMOD_CODEC_SOUNDCREATECALLBACK soundcreate;
        FMOD_CODEC_GETWAVEFORMAT getwaveformat;
};

struct FMOD_CODEC_WAVEFORMAT {
        char name[256];
        FMOD_SOUND_FORMAT format;
        int32_t channels;
        int32_t frequency;
        uint32_t lengthbytes;
        uint32_t lengthpcm;
        int32_t blockalign;
        int32_t loopstart;
        int32_t loopend;
        FMOD_MODE mode;
        uint32_t channelmask;
};

struct FMOD_CODEC_STATE {
        int32_t numsubsounds;
        FMOD_CODEC_WAVEFORMAT* waveformat;
        void* plugindata;
        void* filehandle;
        uint32_t filesize;
        FMOD_FILE_READCALLBACK fileread;
        FMOD_FILE_SEEKCALLBACK fileseek;
        FMOD_CODEC_METADATACALLBACK metadata;
};

struct FMOD_DSP_PARAMETERDESC {
        float min;
        float max;
        float defaultval;
        char name[16];
        char label[16];
        const char* description;
};

struct FMOD_DSP_DESCRIPTION {
        char name[32];
        uint32_t version;
        int32_t channels;
        FMOD_DSP_CREATECALLBACK create;
        FMOD_DSP_RELEASECALLBACK release;
        FMOD_DSP_RESETCALLBACK reset;
        FMOD_DSP_READCALLBACK read;
        FMOD_DSP_SETPOSITIONCALLBACK setposition;
        int32_t numparameters;
        FMOD_DSP_PARAMETERDESC* paramdesc;
        FMOD_DSP_SETPARAMCALLBACK setparameter;
        FMOD_DSP_GETPARAMCALLBACK getparameter;
        FMOD_DSP_DIALOGCALLBACK config;
        int32_t configwidth;
        int32_t configheight;
        void* userdata;
};

struct FMOD_DSP_STATE {
        FMOD_DSP* instance;
        void* plugindata;
};

struct FMOD_OUTPUT_DESCRIPTION {
        const char* name;
        uint32_t version;
        int32_t polling;
        FMOD_OUTPUT_GETNUMDRIVERSCALLBACK getnumdrivers;
        FMOD_OUTPUT_GETDRIVERNAMECALLBACK getdrivername;
        FMOD_OUTPUT_GETDRIVERCAPSCALLBACK getdrivercaps;
        FMOD_OUTPUT_INITCALLBACK init;
        FMOD_OUTPUT_CLOSECALLBACK close;
        FMOD_OUTPUT_UPDATECALLBACK update;
        FMOD_OUTPUT_GETHANDLECALLBACK gethandle;
        FMOD_OUTPUT_GETPOSITIONCALLBACK getposition;
        FMOD_OUTPUT_LOCKCALLBACK lock;
        FMOD_OUTPUT_UNLOCKCALLBACK unlock;
};

struct FMOD_OUTPUT_STATE {
        void* plugindata;
        FMOD_OUTPUT_READFROMMIXER readfrommixer;
};

FMOD_RESULT F_API FMOD_Memory_Initialize(void* poolmem, int32_t poollen, FMOD_MEMORY_ALLOCCALLBACK useralloc, FMOD_MEMORY_REALLOCCALLBACK userrealloc, FMOD_MEMORY_FREECALLBACK userfree);

FMOD_RESULT F_API FMOD_Debug_SetLevel(FMOD_DEBUGLEVEL level);

FMOD_RESULT F_API FMOD_Debug_GetLevel(FMOD_DEBUGLEVEL* level);

FMOD_RESULT F_API FMOD_File_SetDiskBusy(int32_t busy);

FMOD_RESULT F_API FMOD_File_GetDiskBusy(int32_t* busy);

FMOD_RESULT F_API FMOD_System_Create(FMOD_SYSTEM** system);

FMOD_RESULT F_API FMOD_System_Release(FMOD_SYSTEM* system);

namespace FMOD {

class System;
class Sound;
class Channel;
class ChannelGroup;
class SoundGroup;
class Reverb;
class DSP;
class Geometry;

inline FMOD_RESULT Memory_Initialize(
    void* poolmem,
    int32_t poollen,
    FMOD_MEMORY_ALLOCCALLBACK useralloc,
    FMOD_MEMORY_REALLOCCALLBACK userrealloc,
    FMOD_MEMORY_FREECALLBACK userfree) {
    return FMOD_Memory_Initialize(poolmem, poollen, useralloc, userrealloc, userfree);
}

inline FMOD_RESULT Debug_SetLevel(FMOD_DEBUGLEVEL level) {
    return FMOD_Debug_SetLevel(level);
}

inline FMOD_RESULT Debug_GetLevel(FMOD_DEBUGLEVEL* level) {
    return FMOD_Debug_GetLevel(level);
}

inline FMOD_RESULT File_SetDiskBusy(int32_t busy) {
    return FMOD_File_SetDiskBusy(busy);
}

inline FMOD_RESULT File_GetDiskBusy(int32_t* busy) {
    return FMOD_File_GetDiskBusy(busy);
}

inline FMOD_RESULT System_Create(System** system) {
    return FMOD_System_Create(reinterpret_cast<FMOD_SYSTEM**>(system));
}

class System {
    private:
        System();

    public:
        FMOD_RESULT F_API release();
        FMOD_RESULT F_API setOutput(FMOD_OUTPUTTYPE output);
        FMOD_RESULT F_API getOutput(FMOD_OUTPUTTYPE* output);
        FMOD_RESULT F_API getNumDrivers(int32_t* numdrivers);
        FMOD_RESULT F_API getDriver(int32_t* driver);
        FMOD_RESULT F_API getDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT F_API getDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode);
        FMOD_RESULT F_API setDriver(int32_t driver);
        FMOD_RESULT F_API setHardwareChannels(int32_t min2d, int32_t max2d, int32_t min3d, int32_t max3d);
        FMOD_RESULT F_API setSoftwareChannels(int32_t numsoftwarechannels);
        FMOD_RESULT F_API getSoftwareChannels(int32_t* numsoftwarechannels);
        FMOD_RESULT F_API setSoftwareFormat(int32_t samplerate, FMOD_SOUND_FORMAT format, int32_t numoutputchannels, int32_t maxinputchannels, FMOD_DSP_RESAMPLER resamplemethod);
        FMOD_RESULT F_API getSoftwareFormat(int32_t* samplerate, FMOD_SOUND_FORMAT* format, int32_t* numoutputchannels, int32_t* maxinputchannels, FMOD_DSP_RESAMPLER* resamplemethod, int32_t* bits);
        FMOD_RESULT F_API setDSPBufferSize(uint32_t bufferlength, int32_t numbuffers);
        FMOD_RESULT F_API setSpeakerMode(FMOD_SPEAKERMODE speakermode);
        FMOD_RESULT F_API setCallback(FMOD_SYSTEM_CALLBACKTYPE type, FMOD_SYSTEM_CALLBACK callback);
        FMOD_RESULT F_API init(int32_t maxchannels, FMOD_INITFLAGS flags, void* extradriverdata);
        FMOD_RESULT F_API close();
        FMOD_RESULT F_API update();
        FMOD_RESULT F_API set3DSettings(float dopplerscale, float distancefactor, float rolloffscale);
        FMOD_RESULT F_API set3DListenerAttributes(int32_t listener, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel, const FMOD_VECTOR* forward, const FMOD_VECTOR* up);
        FMOD_RESULT F_API get3DListenerAttributes(int32_t listener, FMOD_VECTOR* pos, FMOD_VECTOR* vel, FMOD_VECTOR* forward, FMOD_VECTOR* up);
        FMOD_RESULT F_API set3DRolloffCallback(FMOD_3D_ROLLOFFCALLBACK callback);
        FMOD_RESULT F_API getVersion(uint32_t* version);
        FMOD_RESULT F_API getHardwareChannels(int32_t* num2d, int32_t* num3d, int32_t* total);
        FMOD_RESULT F_API createSound(const char* name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO* exinfo, Sound** sound);
        FMOD_RESULT F_API createStream(const char* name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO* exinfo, Sound** sound);
        FMOD_RESULT F_API createDSP(FMOD_DSP_DESCRIPTION* description, DSP** dsp);
        FMOD_RESULT F_API createDSPByType(FMOD_DSP_TYPE type, DSP** dsp);
        FMOD_RESULT F_API createChannelGroup(const char* name, ChannelGroup** channelgroup);
        FMOD_RESULT F_API createSoundGroup(const char* name, SoundGroup** soundgroup);
        FMOD_RESULT F_API playSound(FMOD_CHANNELINDEX channelid, Sound* sound, bool paused, Channel** channel);
        FMOD_RESULT F_API playDSP(FMOD_CHANNELINDEX channelid, DSP* dsp, bool paused, Channel** channel);
        FMOD_RESULT F_API getChannel(int32_t channelid, Channel** channel);
        FMOD_RESULT F_API getMasterChannelGroup(ChannelGroup** channelgroup);
        FMOD_RESULT F_API setReverbProperties(FMOD_REVERB_PROPERTIES* prop);
        FMOD_RESULT F_API setRecordDriver(int32_t recorddriver);
        FMOD_RESULT F_API getRecordNumDrivers(int32_t* numdrivers);
        FMOD_RESULT F_API getRecordDriverName(int32_t id, char* name, int32_t namelen);
        FMOD_RESULT F_API getRecordPosition(uint32_t* position);
        FMOD_RESULT F_API recordStart(Sound* sound, bool loop);
        FMOD_RESULT F_API recordStop();
        FMOD_RESULT F_API isRecording(bool* recording);
};

class Sound {
    private:
        Sound();

    public:
        FMOD_RESULT F_API readyForRelease(bool* ready);
        FMOD_RESULT F_API release();
        FMOD_RESULT F_API lock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2);
        FMOD_RESULT F_API unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2);
        FMOD_RESULT F_API getName(char* name, int32_t namelen);
        FMOD_RESULT F_API getLength(uint32_t* length, FMOD_TIMEUNIT lengthtype);
        FMOD_RESULT F_API setSoundGroup(SoundGroup* soundgroup);
        FMOD_RESULT F_API getUserData(void** _userdata);
        FMOD_RESULT F_API set3DMinMaxDistance(float min, float max);
        FMOD_RESULT F_API setLoopCount(int32_t loopcount);
        FMOD_RESULT F_API getLoopCount(int32_t* loopcount);
};

class Channel {
    private:
        Channel();

    public:
        FMOD_RESULT F_API stop();
        FMOD_RESULT F_API setPaused(bool paused);
        FMOD_RESULT F_API setVolume(float volume);
        FMOD_RESULT F_API getVolume(float* volume);
        FMOD_RESULT F_API setFrequency(float frequency);
        FMOD_RESULT F_API getFrequency(float* frequency);
        FMOD_RESULT F_API setSpeakerMix(float frontleft, float frontright, float center, float lfe, float backleft, float backright, float sideleft, float sideright);
        FMOD_RESULT F_API setPriority(int32_t priority);
        FMOD_RESULT F_API getPriority(int32_t* priority);
        FMOD_RESULT F_API setPosition(uint32_t position, FMOD_TIMEUNIT postype);
        FMOD_RESULT F_API getPosition(uint32_t* position, FMOD_TIMEUNIT postype);
        FMOD_RESULT F_API setReverbProperties(const FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT F_API getReverbProperties(FMOD_REVERB_CHANNELPROPERTIES* prop);
        FMOD_RESULT F_API setChannelGroup(ChannelGroup* channelgroup);
        FMOD_RESULT F_API setCallback(FMOD_CHANNEL_CALLBACKTYPE type, FMOD_CHANNEL_CALLBACK callback, int32_t command);
        FMOD_RESULT F_API set3DAttributes(const FMOD_VECTOR* pos, const FMOD_VECTOR* vel);
        FMOD_RESULT F_API get3DAttributes(FMOD_VECTOR* pos, FMOD_VECTOR* vel);
        FMOD_RESULT F_API set3DMinMaxDistance(float mindistance, float maxdistance);
        FMOD_RESULT F_API get3DMinMaxDistance(float* mindistance, float* maxdistance);
        FMOD_RESULT F_API set3DConeSettings(float insideconeangle, float outsideconeangle, float outsidevolume);
        FMOD_RESULT F_API set3DConeOrientation(FMOD_VECTOR* orientation);
        FMOD_RESULT F_API set3DPanLevel(float level);
        FMOD_RESULT F_API get3DPanLevel(float* level);
        FMOD_RESULT F_API addDSP(DSP* dsp);
        FMOD_RESULT F_API isPlaying(bool* isplaying);
        FMOD_RESULT F_API isVirtual(bool* isvirtual);
        FMOD_RESULT F_API getAudibility(float* audibility);
        FMOD_RESULT F_API getFadeVolume(float* fadevolume);
        FMOD_RESULT F_API getMode(FMOD_MODE* mode);
        FMOD_RESULT F_API setUserData(void* _userdata);
        FMOD_RESULT F_API getUserData(void** _userdata);
        FMOD_RESULT F_API setMute(bool mute);
        FMOD_RESULT F_API getMute(bool* mute);
};

class ChannelGroup {
    private:
        ChannelGroup();

    public:
        FMOD_RESULT F_API release();
        FMOD_RESULT F_API setVolume(float volume);
        FMOD_RESULT F_API getDSPHead(DSP** dsp);
        FMOD_RESULT F_API getNumChannels(int32_t* numchannels);
};

class SoundGroup {
    private:
        SoundGroup();

    public:
        FMOD_RESULT F_API release();
        FMOD_RESULT F_API setMaxAudible(int32_t maxaudible);
        FMOD_RESULT F_API setMaxAudibleBehavior(FMOD_SOUNDGROUP_BEHAVIOR behavior);
        FMOD_RESULT F_API setMuteFadeSpeed(float speed);
};

class DSP {
    private:
        DSP();

    public:
        FMOD_RESULT F_API release();
        FMOD_RESULT F_API addInput(DSP* target);
        FMOD_RESULT F_API disconnectAll(bool inputs, bool outputs);
        FMOD_RESULT F_API remove();
        FMOD_RESULT F_API getOutput(int32_t index, DSP** output);
        FMOD_RESULT F_API setActive(bool active);
        FMOD_RESULT F_API setBypass(bool bypass);
        FMOD_RESULT F_API setParameter(int32_t index, float value);
        FMOD_RESULT F_API getUserData(void** _userdata);
};

}; // namespace FMOD

#endif
