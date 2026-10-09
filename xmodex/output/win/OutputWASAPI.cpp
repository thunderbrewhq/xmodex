#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/output/win/OutputWASAPI.hpp"
#include "xmodex/sound/SoundI.hpp"
#include <cstdio>

namespace FMOD {

static const GUID _KSDATAFORMAT_SUBTYPE_PCM = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID _KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

static const PROPERTYKEY PKEY_Device_DeviceDesc = { { 0xa45c254e, 0xdf1c, 0x4efd, { 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0 } }, 2 };
static const PROPERTYKEY PKEY_DeviceInterface_FriendlyName = { { 0x026e516e, 0xb814, 0x414b, { 0x83, 0xcd, 0x85, 0x6d, 0x6f, 0xef, 0x48, 0x22 } }, 2 };

FMOD_OUTPUT_DESCRIPTION_EX wasapioutput;

FMOD_RESULT OutputWASAPI::close() {
    if (this->mRenderAudioClock) {
        this->mRenderAudioClock->Release();
        this->mRenderAudioClock = nullptr;
    }

    if (this->mRenderClient) {
        this->mRenderClient->Release();
        this->mRenderClient = nullptr;
    }

    if (this->mRenderAudioClient) {
        this->mRenderAudioClient->Release();
        this->mRenderAudioClient = nullptr;
    }

    if (this->mCaptureClient) {
        this->mCaptureClient->Release();
        this->mCaptureClient = nullptr;
    }

    if (this->mCaptureAudioClient) {
        this->mCaptureAudioClient->Release();
        this->mCaptureAudioClient = nullptr;
    }

    for (int32_t i = 0; i < this->mNumRenderDrivers; i++) {
        if (this->mRenderDrivers[i].name) {
            FMOD_Memory_Free(this->mRenderDrivers[i].name);
            this->mRenderDrivers[i].name = nullptr;
        }

        CoTaskMemFree(this->mRenderDrivers[i].id);
    }

    for (int32_t i = 0; i < this->mNumCaptureDrivers; i++) {
        if (this->mCaptureDrivers[i].name) {
            FMOD_Memory_Free(this->mCaptureDrivers[i].name);
            this->mCaptureDrivers[i].name = NULL;
        }
        CoTaskMemFree(this->mCaptureDrivers[i].id);
    }

    this->mNumRenderDrivers = 0;
    this->mNumCaptureDrivers = 0;

    if (this->mRecordBuffer) {
        FMOD_Memory_Free(this->mRecordBuffer);
        this->mRecordBuffer = nullptr;
    }

    CoUninitialize();

    FMOD_OS_CriticalSection_Free(this->mRecordCrit);
    this->mRecordCrit = nullptr;

    this->mInitialised = false;
    this->mCoInitialized = false;

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::enumerate() {
    FMOD_RESULT result = FMOD_OK;
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDeviceCollection* collection = NULL;
    IMMDevice* device = NULL;
    IPropertyStore* properties = NULL;
    PROPVARIANT description;
    PROPVARIANT friendlyname;
    UINT numdevices = 0;

    if (!this->mCoInitialized) {
        Plugin::init();

        CoInitialize(NULL);
        this->mCoInitialized = true;
    }

    for (int32_t i = 0; i < this->mNumRenderDrivers; i++) {
        if (this->mRenderDrivers[i].name) {
            FMOD_Memory_Free(this->mRenderDrivers[i].name);
            this->mRenderDrivers[i].name = nullptr;
        }
        CoTaskMemFree(this->mRenderDrivers[i].id);
    }

    for (int32_t i = 0; i < this->mNumCaptureDrivers; i++) {
        if (this->mCaptureDrivers[i].name) {
            FMOD_Memory_Free(this->mCaptureDrivers[i].name);
            this->mCaptureDrivers[i].name = nullptr;
        }
        CoTaskMemFree(this->mCaptureDrivers[i].id);
    }

    this->mNumRenderDrivers = 0;
    this->mNumCaptureDrivers = 0;

    if (CoCreateInstance(
            CLSID_MMDeviceEnumerator,
            nullptr,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            IID_IMMDeviceEnumerator,
            reinterpret_cast<void**>(&enumerator)) < 0) {
        result = FMOD_ERR_COM;
        goto Exit;
    }

    result = this->enumerateDefaultDevices(enumerator);
    if (result != FMOD_OK) {
        goto Exit;
    }

    if (enumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &collection) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (collection->GetCount(&numdevices) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    for (UINT i = 0; i < numdevices; i++) {
        char name[512] = { 0 };
        IMMEndpoint* endpoint = NULL;
        EDataFlow dataflow = eAll;

        if (collection->Item(i, &device) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }
        if (device->OpenPropertyStore(STGM_READ, &properties) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        FMOD_memset(&description, 0, sizeof(PROPVARIANT));
        FMOD_memset(&friendlyname, 0, sizeof(PROPVARIANT));

        if (properties->GetValue(PKEY_Device_DeviceDesc, &description) < 0 || properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyname) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        sprintf(name, "%S (%S)", description.pwszVal, friendlyname.pwszVal);

        device->QueryInterface(IID_IMMEndpoint, reinterpret_cast<void**>(&endpoint));
        endpoint->GetDataFlow(&dataflow);

        if ((dataflow == eRender || dataflow == eAll) && this->mNumRenderDrivers < FMOD_OUTPUT_MAXDRIVERS) {
            if (FMOD_strcmp(name, this->mRenderDrivers[0].name)) {
                this->mRenderDrivers[this->mNumRenderDrivers].name = (char*)FMOD_Memory_Calloc(FMOD_strlen(name) + 1);
                FMOD_strncpy(this->mRenderDrivers[this->mNumRenderDrivers].name, name, FMOD_strlen(name));

                if (device->GetId(&this->mRenderDrivers[this->mNumRenderDrivers].id) < 0) {
                    result = FMOD_ERR_OUTPUT_INIT;
                    goto Exit;
                }

                this->mNumRenderDrivers++;
            }
        }

        if ((dataflow == eCapture || dataflow == eAll) && this->mNumCaptureDrivers < FMOD_OUTPUT_MAXDRIVERS && FMOD_strcmp(name, this->mCaptureDrivers[0].name)) {
            this->mCaptureDrivers[this->mNumCaptureDrivers].name = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(name) + 1));
            FMOD_strncpy(this->mCaptureDrivers[mNumCaptureDrivers].name, name, FMOD_strlen(name));

            if (device->GetId(&this->mCaptureDrivers[this->mNumCaptureDrivers].id) < 0) {
                result = FMOD_ERR_OUTPUT_INIT;
                goto Exit;
            }

            this->mNumCaptureDrivers++;
        }

        PropVariantClear(&description);
        PropVariantClear(&friendlyname);

        if (properties) {
            properties->Release();
            properties = nullptr;
        }
        if (device) {
            device->Release();
            device = nullptr;
        }
    }

    if (collection) {
        collection->Release();
        collection = nullptr;
    }
    if (enumerator) {
        enumerator->Release();
    }

    return FMOD_OK;

Exit:
    PropVariantClear(&friendlyname);
    PropVariantClear(&description);

    if (properties) {
        properties->Release();
        properties = nullptr;
    }
    if (device) {
        device->Release();
        device = nullptr;
    }
    if (collection) {
        collection->Release();
        collection = nullptr;
    }
    if (enumerator) {
        enumerator->Release();
    }

    return result;
}

FMOD_RESULT OutputWASAPI::enumerateDefaultDevices(IMMDeviceEnumerator* pEnumerator) {
    FMOD_RESULT result = FMOD_OK;
    HRESULT hr;
    IMMDevice* device = nullptr;
    IPropertyStore* properties = nullptr;
    PROPVARIANT description;
    PROPVARIANT friendlyname;
    char name[512] = { 0 };

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (hr >= 0) {
        if (device->OpenPropertyStore(STGM_READ, &properties) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        FMOD_memset(&description, 0, sizeof(PROPVARIANT));
        FMOD_memset(&friendlyname, 0, sizeof(PROPVARIANT));

        if (properties->GetValue(PKEY_Device_DeviceDesc, &description) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }
        if (properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyname) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        sprintf(name, "%S (%S)", description.pwszVal, friendlyname.pwszVal);

        this->mRenderDrivers[0].name = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(name) + 1));
        FMOD_strncpy(this->mRenderDrivers[0].name, name, FMOD_strlen(name));

        if (device->GetId(&this->mRenderDrivers[0].id) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        this->mNumRenderDrivers++;

        PropVariantClear(&description);
        PropVariantClear(&friendlyname);

        if (properties) {
            properties->Release();
            properties = nullptr;
        }
        if (device) {
            device->Release();
            device = nullptr;
        }
    } else if (hr != E_NOTFOUND) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
    if (hr >= 0) {
        if (device->OpenPropertyStore(STGM_READ, &properties) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        FMOD_memset(&description, 0, sizeof(PROPVARIANT));
        FMOD_memset(&friendlyname, 0, sizeof(PROPVARIANT));

        if (properties->GetValue(PKEY_Device_DeviceDesc, &description) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }
        if (properties->GetValue(PKEY_DeviceInterface_FriendlyName, &friendlyname) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        sprintf(name, "%S (%S)", description.pwszVal, friendlyname.pwszVal);

        this->mCaptureDrivers[0].name = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(name) + 1));
        FMOD_strncpy(this->mCaptureDrivers[0].name, name, FMOD_strlen(name));

        if (device->GetId(&this->mCaptureDrivers[0].id) < 0) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto Exit;
        }

        this->mNumCaptureDrivers++;
    } else if (hr != E_NOTFOUND) {
        result = FMOD_ERR_OUTPUT_INIT;
    }

Exit:
    PropVariantClear(&description);
    PropVariantClear(&friendlyname);

    if (properties) {
        properties->Release();
        properties = nullptr;
    }
    if (device) {
        device->Release();
    }

    return result;
}

FMOD_RESULT OutputWASAPI::getDriverCapsEx(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode) {
    FMOD_RESULT result;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioclient = nullptr;
    WAVEFORMATEX* mixformat = nullptr;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (CoCreateInstance(
            CLSID_MMDeviceEnumerator,
            nullptr,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            IID_IMMDeviceEnumerator,
            reinterpret_cast<void**>(&enumerator)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else if (enumerator->GetDevice(this->mRenderDrivers[id].id, &device) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else if (device->Activate(
                   IID_IAudioClient,
                   CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                   nullptr,
                   reinterpret_cast<void**>(&audioclient)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else if (audioclient->GetMixFormat(&mixformat) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else {
        switch (mixformat->nChannels) {
        case 4: {
            *controlpanelspeakermode = FMOD_SPEAKERMODE_QUAD;
            break;
        }
        case 6: {
            *controlpanelspeakermode = FMOD_SPEAKERMODE_5POINT1;
            break;
        }
        case 8: {
            *controlpanelspeakermode = FMOD_SPEAKERMODE_7POINT1;
            break;
        }
        default: {
            *controlpanelspeakermode = FMOD_SPEAKERMODE_STEREO;
            break;
        }
        }

        *minfrequency = *maxfrequency = mixformat->nSamplesPerSec;

        *caps = static_cast<FMOD_CAPS>(FMOD_CAPS_OUTPUT_FORMAT_PCM8 | FMOD_CAPS_OUTPUT_FORMAT_PCM16 | FMOD_CAPS_OUTPUT_FORMAT_PCM24 | FMOD_CAPS_OUTPUT_FORMAT_PCM32 | FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
        if (mixformat->nChannels > 2) {
            *caps |= static_cast<FMOD_CAPS>(FMOD_CAPS_OUTPUT_MULTICHANNEL);
        }
    }

    CoTaskMemFree(mixformat);

    if (audioclient) {
        audioclient->Release();
        audioclient = nullptr;
    }
    if (device) {
        device->Release();
        device = nullptr;
    }
    if (enumerator) {
        enumerator->Release();
    }

    return result;
}

FMOD_RESULT OutputWASAPI::getDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;

    if (!name || namelen < 1) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0 || id >= this->mNumRenderDrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(name, this->mRenderDrivers[id].name, namelen - 1);

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::getHandle(void** handle) {
    *handle = this->mRenderClient;

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::getNumDrivers(int32_t* numdrivers) {
    FMOD_RESULT result;

    if (!numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    *numdrivers = this->mNumRenderDrivers;

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::getPosition(uint32_t* pcm) {
    UINT64 position = 0;
    UINT64 frequency = 0;
    UINT32 padding = 0;

    if (this->mRenderAudioClock->GetPosition(&position, nullptr) < 0 ||
        this->mRenderAudioClock->GetFrequency(&frequency) < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    *pcm = static_cast<uint32_t>(this->mSystem->mOutputRate * position / frequency) % this->mRenderBufferLength;

    if (this->mRenderAudioClient->GetCurrentPadding(&padding) < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    if (!padding) {
        FMOD_RESULT result;
        uint32_t bufferlength = 0;
        int32_t numbuffers = 0;

        result = this->mSystem->getDSPBufferSize(&bufferlength, &numbuffers);
        if (result != FMOD_OK) {
            return result;
        }

        this->mFillBlock = (static_cast<int32_t>(*pcm / bufferlength) % numbuffers + 1) % numbuffers;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::initEx(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, FMOD_SPEAKERMODE* speakermode, int32_t dspbufferlength, int32_t dspnumbuffers, int32_t max2dchannels, int32_t max3dchannels, void* extradriverdata) {
    FMOD_RESULT result;
    HRESULT hr;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    WAVEFORMATEX* mixformat = nullptr;
    WAVEFORMATEX* closest = nullptr;
    WAVEFORMATEXTENSIBLE wfx;
    REFERENCE_TIME period = 0;
    int32_t bits;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (!this->mNumRenderDrivers) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (flags & FMOD_INIT_WASAPI_EXCLUSIVE) {
        this->mExclusiveMode = true;
    }

    if (CoCreateInstance(
            CLSID_MMDeviceEnumerator,
            nullptr,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            IID_IMMDeviceEnumerator,
            reinterpret_cast<void**>(&enumerator)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (enumerator->GetDevice(this->mRenderDrivers[selecteddriver].id, &device) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (device->Activate(
            IID_IAudioClient,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            nullptr,
            reinterpret_cast<void**>(&this->mRenderAudioClient)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    FMOD_memset(&wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));

    if (this->mRenderAudioClient->GetMixFormat(&mixformat) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    switch (*outputformat) {
    case FMOD_SOUND_FORMAT_PCM8:
        bits = 8;
        break;
    case FMOD_SOUND_FORMAT_PCM16:
        bits = 16;
        break;
    case FMOD_SOUND_FORMAT_PCM24:
        bits = 24;
        break;
    case FMOD_SOUND_FORMAT_PCM32:
    case FMOD_SOUND_FORMAT_PCMFLOAT:
        bits = 32;
        break;
    case FMOD_SOUND_FORMAT_NONE:
    case FMOD_SOUND_FORMAT_GCADPCM:
    case FMOD_SOUND_FORMAT_IMAADPCM:
    case FMOD_SOUND_FORMAT_VAG:
    case FMOD_SOUND_FORMAT_XMA:
    case FMOD_SOUND_FORMAT_MPEG:
        bits = 0;
        break;
    default: {
        result = FMOD_ERR_FORMAT;
        goto Exit;
    }
    }

    switch (mixformat->nChannels) {
    case 1:
        *speakermode = FMOD_SPEAKERMODE_MONO;
        break;
    case 2:
        *speakermode = FMOD_SPEAKERMODE_STEREO;
        break;
    case 4:
        *speakermode = FMOD_SPEAKERMODE_QUAD;
        break;
    case 6:
        *speakermode = FMOD_SPEAKERMODE_5POINT1;
        break;
    case 8:
        *speakermode = FMOD_SPEAKERMODE_7POINT1;
        break;
    }

    wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels = mixformat->nChannels;
    wfx.Format.nSamplesPerSec = *outputrate;
    wfx.Format.nBlockAlign = wfx.Format.nChannels * bits / 8;
    wfx.Format.wBitsPerSample = bits;
    wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    wfx.Format.cbSize = 22;
    wfx.Samples.wValidBitsPerSample = bits;
    FMOD_memcpy(&wfx.SubFormat, (*outputformat == FMOD_SOUND_FORMAT_PCMFLOAT ? &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : &_KSDATAFORMAT_SUBTYPE_PCM), sizeof(GUID));

    hr = this->mRenderAudioClient->IsFormatSupported(
        this->mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
        &wfx.Format, &closest);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == S_FALSE) {
        *outputrate = mixformat->nSamplesPerSec;

        wfx.Format.nSamplesPerSec = mixformat->nSamplesPerSec;
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    }

    if (this->mRenderAudioClient->GetDevicePeriod(
            this->mExclusiveMode ? nullptr : &period,
            this->mExclusiveMode ? &period : nullptr) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    hr = this->mRenderAudioClient->Initialize(
        this->mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
        0,
        static_cast<REFERENCE_TIME>(dspbufferlength * dspnumbuffers) * 10000000 / wfx.Format.nSamplesPerSec,
        this->mExclusiveMode ? period : 0,
        &wfx.Format,
        nullptr);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
        result = FMOD_ERR_OUTPUT_FORMAT;
        goto Exit;
    }
    if (hr < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    if (this->mRenderAudioClient->GetBufferSize(&this->mRenderBufferLength) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (this->mRenderAudioClient->GetService(IID_IAudioRenderClient, reinterpret_cast<void**>(&this->mRenderClient)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (this->mRenderAudioClient->GetService(IID_IAudioClock, reinterpret_cast<void**>(&this->mRenderAudioClock)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (this->mRenderClient->GetBuffer(this->mRenderBufferLength, &this->mRenderBuffer) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (this->mRenderClient->ReleaseBuffer(this->mRenderBufferLength, 0) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    result = FMOD_OS_CriticalSection_Create(&this->mRecordCrit);
    if (result == FMOD_OK) {
        mInitialised = true;
    }

Exit:
    CoTaskMemFree(closest);
    CoTaskMemFree(mixformat);

    if (device) {
        device->Release();
        device = NULL;
    }
    if (enumerator) {
        enumerator->Release();
    }

    return result;
}

FMOD_RESULT OutputWASAPI::lock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    FMOD_RESULT result;
    UINT32 padding = 0;
    unsigned int samples;
    FMOD_SOUND_FORMAT format;
    int32_t channels;

    if (this->mRenderAudioClient->GetCurrentPadding(&padding) < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    format = this->mSystem->mOutputFormat;
    channels = this->mSystem->mMaxOutputChannels;

    result = SoundI::getSamplesFromBytes(length, &samples, channels, format);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mRenderBufferLength - padding <= samples) {
        *ptr1 = nullptr;
        *len1 = 0;
        *ptr2 = nullptr;
        *len2 = 0;
        return FMOD_OK;
    }

    if (this->mRenderClient->GetBuffer(samples, reinterpret_cast<BYTE**>(ptr1)) < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    *len1 = length;
    *ptr2 = nullptr;
    *len2 = 0;

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::recordGetDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency) {
    FMOD_RESULT result;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioclient = nullptr;
    WAVEFORMATEX* mixformat = nullptr;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (CoCreateInstance(
            CLSID_MMDeviceEnumerator,
            nullptr,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            IID_IMMDeviceEnumerator,
            reinterpret_cast<void**>(&enumerator)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else if (enumerator->GetDevice(this->mCaptureDrivers[id].id, &device) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else if (device->Activate(
                   IID_IAudioClient,
                   CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
                   nullptr,
                   reinterpret_cast<void**>(&audioclient)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else if (audioclient->GetMixFormat(&mixformat) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    } else {
        *minfrequency = *maxfrequency = mixformat->nSamplesPerSec;

        *caps = static_cast<FMOD_CAPS>(FMOD_CAPS_OUTPUT_FORMAT_PCM8 | FMOD_CAPS_OUTPUT_FORMAT_PCM16 | FMOD_CAPS_OUTPUT_FORMAT_PCM24 | FMOD_CAPS_OUTPUT_FORMAT_PCM32 | FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);
        if (mixformat->nChannels > 2) {
            *caps |= static_cast<FMOD_CAPS>(FMOD_CAPS_OUTPUT_MULTICHANNEL);
        }
    }

    CoTaskMemFree(mixformat);

    if (audioclient) {
        audioclient->Release();
        audioclient = NULL;
    }
    if (device) {
        device->Release();
        device = NULL;
    }
    if (enumerator) {
        enumerator->Release();
    }

    return result;
}

FMOD_RESULT OutputWASAPI::recordGetDriverInfo(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;

    if (!name || namelen < 1) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0 || id >= this->mNumCaptureDrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(name, this->mCaptureDrivers[id].name, namelen - 1);

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::recordGetNumDrivers(int32_t* numdrivers) {
    FMOD_RESULT result;

    if (!numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    *numdrivers = this->mNumCaptureDrivers;

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::recordLock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    auto lenbytes = this->mRecordBufferLength * this->mRecordBlockAlign;

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

    *ptr1 = this->mRecordBuffer + offset;

    if (offset + length > lenbytes) {
        *len1 = lenbytes - offset;
        *ptr2 = this->mRecordBuffer;
        *len2 = length + offset - lenbytes;
    } else {
        *len1 = length;
        *ptr2 = nullptr;
        *len2 = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::recordStart(int32_t id, Sound* sound, bool loop) {
    FMOD_RESULT result;
    HRESULT hr;
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    WAVEFORMATEX* mixformat = nullptr;
    WAVEFORMATEX* closest = nullptr;
    WAVEFORMATEXTENSIBLE wfx;
    REFERENCE_TIME period = 0;
    auto soundi = reinterpret_cast<SoundI*>(sound);
    int32_t bits = 0;

    if (!soundi || !soundi->mLength) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (!this->mNumCaptureDrivers) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (CoCreateInstance(
            CLSID_MMDeviceEnumerator,
            nullptr,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            IID_IMMDeviceEnumerator,
            reinterpret_cast<void**>(&enumerator)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (enumerator->GetDevice(this->mCaptureDrivers[id].id, &device) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (device->Activate(
            IID_IAudioClient,
            CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
            nullptr,
            reinterpret_cast<void**>(&this->mCaptureAudioClient)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    FMOD_memset(&wfx, 0, sizeof(WAVEFORMATEX));

    if (this->mCaptureAudioClient->GetMixFormat(&mixformat) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    switch (soundi->mFormat) {
    case FMOD_SOUND_FORMAT_PCM8:
        bits = 8;
        break;
    case FMOD_SOUND_FORMAT_PCM16:
        bits = 16;
        break;
    case FMOD_SOUND_FORMAT_PCM24:
        bits = 24;
        break;
    case FMOD_SOUND_FORMAT_PCM32:
    case FMOD_SOUND_FORMAT_PCMFLOAT:
        bits = 32;
        break;
    case FMOD_SOUND_FORMAT_NONE:
    case FMOD_SOUND_FORMAT_GCADPCM:
    case FMOD_SOUND_FORMAT_IMAADPCM:
    case FMOD_SOUND_FORMAT_VAG:
    case FMOD_SOUND_FORMAT_XMA:
    case FMOD_SOUND_FORMAT_MPEG:
        bits = 0;
        break;
    default: {
        result = FMOD_ERR_FORMAT;
        goto Exit;
    }
    }

    wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wfx.Format.nChannels = soundi->mChannels;
    wfx.Format.nSamplesPerSec = static_cast<DWORD>(soundi->mDefaultFrequency);
    wfx.Format.nBlockAlign = wfx.Format.nChannels * bits / 8;
    wfx.Format.wBitsPerSample = bits;
    wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
    wfx.Format.cbSize = 22;
    wfx.Samples.wValidBitsPerSample = bits;
    FMOD_memcpy(&wfx.SubFormat, (soundi->mFormat == FMOD_SOUND_FORMAT_PCMFLOAT ? &_KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : &_KSDATAFORMAT_SUBTYPE_PCM), sizeof(GUID));

    if (this->mCaptureAudioClient->GetDevicePeriod(this->mExclusiveMode ? nullptr : &period, this->mExclusiveMode ? &period : nullptr) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    this->mRecordStereoToMono = false;

    hr = this->mCaptureAudioClient->IsFormatSupported(
        this->mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED,
        reinterpret_cast<WAVEFORMATEX*>(&wfx.Format), &closest);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT || hr == S_FALSE) {
        if (soundi->mChannels == 1 && mixformat->nChannels == 2) {
            wfx.Format.nChannels = 2;
            wfx.Format.nBlockAlign = 2 * wfx.Format.wBitsPerSample / 8;
            wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
            this->mRecordStereoToMono = true;
        }

        if (mixformat->nSamplesPerSec != soundi->mDefaultFrequency) {
            wfx.Format.nSamplesPerSec = mixformat->nSamplesPerSec;
            wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        }
    }

    hr = this->mCaptureAudioClient->Initialize(this->mExclusiveMode ? AUDCLNT_SHAREMODE_EXCLUSIVE : AUDCLNT_SHAREMODE_SHARED, 0, 5000000, this->mExclusiveMode ? period : 0, &wfx.Format, nullptr);
    if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT) {
        result = FMOD_ERR_FORMAT;
        goto Exit;
    }
    if (hr < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    this->mRecordRate = wfx.Format.nSamplesPerSec;
    this->mRecordFormat = soundi->mFormat;
    this->mRecordBlockAlign = soundi->mChannels * (bits / 8);

    if (this->mCaptureAudioClient->GetBufferSize(&this->mRecordBufferLength) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }
    if (this->mCaptureAudioClient->GetService(IID_IAudioCaptureClient, reinterpret_cast<void**>(&this->mCaptureClient)) < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
        goto Exit;
    }

    this->mRecordBuffer = static_cast<uint8_t*>(FMOD_Memory_Calloc(this->mRecordBlockAlign * this->mRecordBufferLength));
    if (!this->mRecordBuffer) {
        result = FMOD_ERR_MEMORY;
        goto Exit;
    }

    this->mRecordTimerPeriod = period * 0.0001f;
    this->mRecordWritePosition = 0;

    this->mRecordTimerID = timeSetEvent(static_cast<UINT>(this->mRecordTimerPeriod), 0, recordUpdateCallback, reinterpret_cast<DWORD_PTR>(this), TIME_PERIODIC);
    if (!this->mRecordTimerID) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (this->mCaptureAudioClient->Start() < 0) {
        result = FMOD_ERR_OUTPUT_INIT;
    }

Exit:
    CoTaskMemFree(closest);
    CoTaskMemFree(mixformat);

    if (device) {
        device->Release();
        device = nullptr;
    }
    if (enumerator) {
        enumerator->Release();
    }

    return result;
}

FMOD_RESULT OutputWASAPI::recordStop() {
    if (this->mCaptureAudioClient) {
        timeKillEvent(this->mRecordTimerID);
        this->mRecordTimerID = 0;

        FMOD_OS_CriticalSection_Enter(this->mRecordCrit);

        this->mCaptureAudioClient->Stop();

        if (this->mCaptureClient) {
            this->mCaptureClient->Release();
            this->mCaptureClient = nullptr;
        }

        if (this->mCaptureAudioClient) {
            this->mCaptureAudioClient->Release();
            this->mCaptureAudioClient = nullptr;
        }

        if (this->mRecordBuffer) {
            FMOD_Memory_Free(this->mRecordBuffer);
            this->mRecordBuffer = nullptr;
        }

        FMOD_OS_CriticalSection_Leave(this->mRecordCrit);
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::recordUpdate() {
    UINT32 packetsize = 0;
    UINT32 numframes = 0;
    DWORD flags = 0;
    BYTE* src = nullptr;

    if (!this->mRecording) {
        return FMOD_OK;
    }

    if (this->mCaptureClient->GetNextPacketSize(&packetsize) < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    while (packetsize) {
        auto writeposition = this->mRecordWritePosition;
        auto dest = this->mRecordBuffer + writeposition * this->mRecordBlockAlign;

        if (this->mCaptureClient->GetBuffer(&src, &numframes, &flags, nullptr, nullptr) < 0) {
            return FMOD_ERR_OUTPUT_DRIVERCALL;
        }

        if (this->mRecordStereoToMono) {
            auto blockalign = this->mRecordBlockAlign;

            for (int32_t count = 0; count <= static_cast<int32_t>(numframes); count++) {
                FMOD_memcpy(dest, src, blockalign);

                src += blockalign * 2;
                dest += blockalign;

                if (dest == this->mRecordBuffer + this->mRecordBufferLength * this->mRecordBlockAlign) {
                    dest = this->mRecordBuffer;
                }
            }
        } else if (writeposition + numframes > this->mRecordBufferLength) {
            uint32_t firstlength = this->mRecordBufferLength - writeposition;

            FMOD_memcpy(dest, src, firstlength * this->mRecordBlockAlign);
            src += firstlength * this->mRecordBlockAlign;
            FMOD_memcpy(this->mRecordBuffer, src, (numframes - firstlength) * this->mRecordBlockAlign);
        } else {
            FMOD_memcpy(dest, src, numframes * this->mRecordBlockAlign);
        }

        if (this->mCaptureClient->ReleaseBuffer(numframes) < 0) {
            return FMOD_ERR_OUTPUT_DRIVERCALL;
        }

        writeposition = (writeposition + numframes) % this->mRecordBufferLength;

        if (this->mCaptureClient->GetNextPacketSize(&packetsize) < 0) {
            return FMOD_ERR_OUTPUT_DRIVERCALL;
        }

        this->mRecordWritePosition = writeposition;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::start() {
    FMOD_RESULT result;

    result = OutputTimer::start();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mRenderAudioClient->Start() < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::stop() {
    FMOD_RESULT result;

    if (!this->mInitialised) {
        return FMOD_OK;
    }

    result = OutputTimer::stop();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mRenderAudioClient->Stop() < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputWASAPI::unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    FMOD_RESULT result;
    uint32_t samples;

    result = SoundI::getSamplesFromBytes(len1, &samples, this->mSystem->mMaxOutputChannels, this->mSystem->mOutputFormat);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mRenderClient->ReleaseBuffer(samples, 0) < 0) {
        return FMOD_ERR_OUTPUT_DRIVERCALL;
    }

    return FMOD_OK;
}

void F_CALLBACK OutputWASAPI::recordUpdateCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
    auto wasapi = reinterpret_cast<OutputWASAPI*>(dwUser);

    FMOD_OS_CriticalSection_Enter(wasapi->mRecordCrit);

    wasapi->recordUpdate();

    FMOD_OS_CriticalSection_Leave(wasapi->mRecordCrit);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->close();
}

FMOD_RESULT F_CALLBACK OutputWASAPI::getDriverCapsExCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->getDriverCapsEx(id, caps, minfrequency, maxfrequency, controlpanelspeakermode);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->getHandle(handle);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::getPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->getPosition(pcm);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::initExCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, FMOD_SPEAKERMODE* speakermode, int32_t dspbufferlength, int32_t dspnumbuffers, int32_t max2dchannels, int32_t max3dchannels, void* extradriverdata) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->initEx(selecteddriver, flags, outputrate, outputchannels, outputformat, speakermode, dspbufferlength, dspnumbuffers, max2dchannels, max3dchannels, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::lockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->lock(offset, length, ptr1, ptr2, len1, len2);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->recordGetDriverCaps(id, caps, minfrequency, maxfrequency);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetDriverInfoCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->recordGetDriverInfo(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->recordGetNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::recordGetPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->getPosition(pcm);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::recordLockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->recordLock(offset, length, ptr1, ptr2, len1, len2);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::recordStartCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_SOUND* sound, int32_t loop) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->recordStart(id, reinterpret_cast<Sound*>(sound), loop);
}

FMOD_RESULT F_CALLBACK OutputWASAPI::recordStopCallback(FMOD_OUTPUT_STATE* output) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->recordStop();
}

FMOD_RESULT F_CALLBACK OutputWASAPI::startCallback(FMOD_OUTPUT_STATE* output) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->start();
}

FMOD_RESULT F_CALLBACK OutputWASAPI::stopCallback(FMOD_OUTPUT_STATE* output) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->stop();
}

FMOD_RESULT F_CALLBACK OutputWASAPI::unlockCallback(FMOD_OUTPUT_STATE* output, void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    auto wasapi = static_cast<OutputWASAPI*>(output);
    return wasapi->unlock(ptr1, ptr2, len1, len2);
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputWASAPI::getDescriptionEx() {
    FMOD_memset(&wasapioutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    wasapioutput.name = "FMOD WASAPI Output";
    wasapioutput.version = 0x10100;
    wasapioutput.mType = FMOD_OUTPUTTYPE_WASAPI;
    wasapioutput.mSize = sizeof(OutputWASAPI);
    wasapioutput.polling = false;

    wasapioutput.getnumdrivers = OutputWASAPI::getNumDriversCallback;
    wasapioutput.getdrivername = OutputWASAPI::getDriverNameCallback;
    wasapioutput.getdrivercapsex = OutputWASAPI::getDriverCapsExCallback;
    wasapioutput.initex = OutputWASAPI::initExCallback;
    wasapioutput.close = OutputWASAPI::closeCallback;
    wasapioutput.getposition = OutputWASAPI::getPositionCallback;
    wasapioutput.lock = OutputWASAPI::lockCallback;
    wasapioutput.unlock = OutputWASAPI::unlockCallback;
    wasapioutput.start = OutputWASAPI::startCallback;
    wasapioutput.stop = OutputWASAPI::stopCallback;
    wasapioutput.gethandle = OutputWASAPI::getHandleCallback;

    wasapioutput.record_getnumdrivers = OutputWASAPI::recordGetNumDriversCallback;
    wasapioutput.record_getdriverinfo = OutputWASAPI::recordGetDriverInfoCallback;
    wasapioutput.record_getdrivercaps = OutputWASAPI::recordGetDriverCapsCallback;
    wasapioutput.record_start = OutputWASAPI::recordStartCallback;
    wasapioutput.record_stop = OutputWASAPI::recordStopCallback;
    wasapioutput.record_getposition = OutputWASAPI::recordGetPositionCallback;
    wasapioutput.record_lock = OutputWASAPI::recordLockCallback;

    return &wasapioutput;
}

} // namespace FMOD
