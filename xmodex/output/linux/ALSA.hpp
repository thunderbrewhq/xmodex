#ifndef XMODEX_OUTPUT_LINUX_ALSA_HPP
#define XMODEX_OUTPUT_LINUX_ALSA_HPP

#include <cstddef>
#include <cstdint>
#include <unistd.h>

#define SND_PCM_NONBLOCK 0x00000001
#define SND_PCM_ASYNC 0x00000002
#define SND_PCM_ABORT 0x00008000
#define SND_PCM_NO_AUTO_RESAMPLE 0x00010000
#define SND_PCM_NO_AUTO_CHANNELS 0x00020000
#define SND_PCM_NO_AUTO_FORMAT 0x00040000
#define SND_PCM_NO_SOFTVOL 0x00080000

enum snd_pcm_access_t {
    SND_PCM_ACCESS_MMAP_INTERLEAVED = 0,
    SND_PCM_ACCESS_MMAP_NONINTERLEAVED,
    SND_PCM_ACCESS_MMAP_COMPLEX,
    SND_PCM_ACCESS_RW_INTERLEAVED,
    SND_PCM_ACCESS_RW_NONINTERLEAVED,
    SND_PCM_ACCESS_LAST = SND_PCM_ACCESS_RW_NONINTERLEAVED
};

enum snd_pcm_format_t {
    SND_PCM_FORMAT_UNKNOWN = -1,
    SND_PCM_FORMAT_S8 = 0,
    SND_PCM_FORMAT_U8,
    SND_PCM_FORMAT_S16_LE,
    SND_PCM_FORMAT_S16_BE,
    SND_PCM_FORMAT_U16_LE,
    SND_PCM_FORMAT_U16_BE,
    SND_PCM_FORMAT_S24_LE,
    SND_PCM_FORMAT_S24_BE,
    SND_PCM_FORMAT_U24_LE,
    SND_PCM_FORMAT_U24_BE,
    SND_PCM_FORMAT_S32_LE,
    SND_PCM_FORMAT_S32_BE,
    SND_PCM_FORMAT_U32_LE,
    SND_PCM_FORMAT_U32_BE,
    SND_PCM_FORMAT_FLOAT_LE,
    SND_PCM_FORMAT_FLOAT_BE,
    SND_PCM_FORMAT_FLOAT64_LE,
    SND_PCM_FORMAT_FLOAT64_BE,
    SND_PCM_FORMAT_IEC958_SUBFRAME_LE,
    SND_PCM_FORMAT_IEC958_SUBFRAME_BE,
    SND_PCM_FORMAT_MU_LAW,
    SND_PCM_FORMAT_A_LAW,
    SND_PCM_FORMAT_IMA_ADPCM,
    SND_PCM_FORMAT_MPEG,
    SND_PCM_FORMAT_GSM,
    SND_PCM_FORMAT_S20_LE,
    SND_PCM_FORMAT_S20_BE,
    SND_PCM_FORMAT_U20_LE,
    SND_PCM_FORMAT_U20_BE,
    SND_PCM_FORMAT_SPECIAL = 31,
    SND_PCM_FORMAT_S24_3LE = 32,
    SND_PCM_FORMAT_S24_3BE,
    SND_PCM_FORMAT_U24_3LE,
    SND_PCM_FORMAT_U24_3BE,
    SND_PCM_FORMAT_S20_3LE,
    SND_PCM_FORMAT_S20_3BE,
    SND_PCM_FORMAT_U20_3LE,
    SND_PCM_FORMAT_U20_3BE,
    SND_PCM_FORMAT_S18_3LE,
    SND_PCM_FORMAT_S18_3BE,
    SND_PCM_FORMAT_U18_3LE,
    SND_PCM_FORMAT_U18_3BE,
    SND_PCM_FORMAT_G723_24,
    SND_PCM_FORMAT_G723_24_1B,
    SND_PCM_FORMAT_G723_40,
    SND_PCM_FORMAT_G723_40_1B,
    SND_PCM_FORMAT_DSD_U8,
    SND_PCM_FORMAT_DSD_U16_LE,
    SND_PCM_FORMAT_DSD_U32_LE,
    SND_PCM_FORMAT_DSD_U16_BE,
    SND_PCM_FORMAT_DSD_U32_BE,
    SND_PCM_FORMAT_LAST = SND_PCM_FORMAT_DSD_U32_BE,

#if defined(XMODEX_CPU_LITTLEENDIAN)
    SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_LE,
    SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_LE,
    SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_LE,
    SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_LE,
    SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_LE,
    SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_LE,
    SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_LE,
    SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_LE,
    SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_LE,
    SND_PCM_FORMAT_S20 = SND_PCM_FORMAT_S20_LE,
    SND_PCM_FORMAT_U20 = SND_PCM_FORMAT_U20_LE,
#elif defined(XMODEX_CPU_BIGENDIAN)
    SND_PCM_FORMAT_S16 = SND_PCM_FORMAT_S16_BE,
    SND_PCM_FORMAT_U16 = SND_PCM_FORMAT_U16_BE,
    SND_PCM_FORMAT_S24 = SND_PCM_FORMAT_S24_BE,
    SND_PCM_FORMAT_U24 = SND_PCM_FORMAT_U24_BE,
    SND_PCM_FORMAT_S32 = SND_PCM_FORMAT_S32_BE,
    SND_PCM_FORMAT_U32 = SND_PCM_FORMAT_U32_BE,
    SND_PCM_FORMAT_FLOAT = SND_PCM_FORMAT_FLOAT_BE,
    SND_PCM_FORMAT_FLOAT64 = SND_PCM_FORMAT_FLOAT64_BE,
    SND_PCM_FORMAT_IEC958_SUBFRAME = SND_PCM_FORMAT_IEC958_SUBFRAME_BE,
    SND_PCM_FORMAT_S20 = SND_PCM_FORMAT_S20_BE,
    SND_PCM_FORMAT_U20 = SND_PCM_FORMAT_U20_BE,
#else
#error "Unknown endian"
#endif
};

enum snd_pcm_stream_t {
    SND_PCM_STREAM_PLAYBACK = 0,
    SND_PCM_STREAM_CAPTURE,
    SND_PCM_STREAM_LAST = SND_PCM_STREAM_CAPTURE
};

typedef uint32_t snd_pcm_uframes_t;
typedef int32_t snd_pcm_sframes_t;

struct snd_pcm_t;
struct snd_pcm_hw_params_t;

typedef int32_t (*snd_pcm_open_t)(
    snd_pcm_t** pcm,
    const char* name,
    snd_pcm_stream_t stream,
    int32_t mode);

typedef size_t (*snd_pcm_hw_params_sizeof_t)(void);

typedef int32_t (*snd_pcm_hw_params_any_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params);

typedef int32_t (*snd_pcm_hw_params_set_access_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    snd_pcm_access_t access);

typedef int32_t (*snd_pcm_hw_params_set_format_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    snd_pcm_format_t val);

typedef int32_t (*snd_pcm_hw_params_set_rate_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    uint32_t val,
    int32_t dir);

typedef int32_t (*snd_pcm_hw_params_set_rate_near_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    uint32_t* val,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_set_channels_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    uint32_t val);

typedef int32_t (*snd_pcm_hw_params_set_periods_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    uint32_t val,
    int32_t dir);

typedef int32_t (*snd_pcm_hw_params_set_periods_near_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    uint32_t* val,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_set_period_time_near_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    uint32_t* val,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_set_buffer_size_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    snd_pcm_uframes_t val);

typedef int32_t (*snd_pcm_hw_params_set_buffer_size_near_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    snd_pcm_uframes_t* val);

typedef int32_t (*snd_pcm_hw_params_set_buffer_time_near_t)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params,
    uint32_t* val,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_get_buffer_size_t)(
    const snd_pcm_hw_params_t* params,
    snd_pcm_uframes_t* val);

typedef int32_t (*snd_pcm_hw_params_get_buffer_size_min_t)(
    const snd_pcm_hw_params_t* params,
    snd_pcm_uframes_t* val);

typedef int32_t (*snd_pcm_hw_params_get_buffer_size_max_t)(
    const snd_pcm_hw_params_t* params,
    snd_pcm_uframes_t* val);

typedef int32_t (*snd_pcm_hw_params_get_buffer_time_min_t)(
    const snd_pcm_hw_params_t* params,
    uint32_t* val,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_get_buffer_time_max_t)(
    const snd_pcm_hw_params_t* params,
    uint32_t* val,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_get_period_time_t)(
    const snd_pcm_hw_params_t* params,
    uint32_t* val,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_get_period_size_t)(
    const snd_pcm_hw_params_t* params,
    snd_pcm_uframes_t* frames,
    int32_t* dir);

typedef int32_t (*snd_pcm_hw_params_t_fn)(
    snd_pcm_t* pcm,
    snd_pcm_hw_params_t* params);

typedef snd_pcm_sframes_t (*snd_pcm_writei_t)(
    snd_pcm_t* pcm,
    const void* buffer,
    snd_pcm_uframes_t size);

typedef snd_pcm_sframes_t (*snd_pcm_readi_t)(
    snd_pcm_t* pcm,
    void* buffer,
    snd_pcm_uframes_t size);

typedef int32_t (*snd_pcm_prepare_t)(
    snd_pcm_t* pcm);

typedef int32_t (*snd_pcm_close_t)(
    snd_pcm_t* pcm);

typedef int32_t (*snd_pcm_drop_t)(
    snd_pcm_t* pcm);

typedef int32_t (*snd_card_next_t)(
    int32_t* rcard);

typedef int32_t (*snd_card_get_name_t)(
    int32_t card,
    char** name);

typedef int32_t (*snd_pcm_nonblock_t)(
    snd_pcm_t* pcm,
    int32_t nonblock);

typedef const char* (*snd_strerror_t)(
    int32_t errnum);

typedef snd_pcm_sframes_t (*snd_pcm_bytes_to_frames_t)(
    snd_pcm_t* pcm,
    ssize_t bytes);

typedef ssize_t (*snd_pcm_frames_to_bytes_t)(
    snd_pcm_t* pcm,
    snd_pcm_sframes_t frames);

#endif
