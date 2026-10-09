#include "xmodex/output/mac/OutputCoreAudio.hpp"
#include "fmod.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/Memory.hpp"
#include <CoreAudio/AudioHardware.h>
#include <CoreAudio/AudioHardwareBase.h>

namespace FMOD {

FMOD_OUTPUT_DESCRIPTION_EX coreaudiooutput;

OSStatus OutputCoreAudio::renderProc(
    void* inrefcon,
    AudioUnitRenderActionFlags* ioactionflags,
    const AudioTimeStamp* intimestamp,
    UInt32 inbusnumber,
    UInt32 innumberframes,
    AudioBufferList* iodata) {

    FMOD_RESULT result;
    OSStatus status = -1;

    auto coreaudio = static_cast<OutputCoreAudio*>(inrefcon);
    result = coreaudio->mix(iodata->mBuffers[0].mData, innumberframes);

    coreaudio->mSystem->mDSPTimeStamp.stampIn();
    if (result == FMOD_OK) {
        coreaudio->mSystem->mDSPTimeStamp.stampOut(95);
        status = 0;
    }

    return status;
}

OSStatus OutputCoreAudio::recordProc(
    void* inrefcon,
    AudioUnitRenderActionFlags* ioactionflags,
    const AudioTimeStamp* intimestamp,
    UInt32 inbusnumber,
    UInt32 innumberframes,
    AudioBufferList* iodata) {

    auto coreaudio = static_cast<OutputCoreAudio*>(inrefcon);

    if (coreaudio->mRecording) {
        coreaudio->mRecordBufferList.mBuffers[0].mData =
            static_cast<uint8_t*>(coreaudio->mRecordBuffer) +
            (coreaudio->mRecordMaxFramesPerSlice *
             coreaudio->mRecordNumBytesPerFrame *
             coreaudio->mRecordSliceIndex);

        if (AudioUnitRender(
                coreaudio->mInputUnit,
                ioactionflags,
                intimestamp,
                inbusnumber,
                innumberframes,
                &coreaudio->mRecordBufferList)) {
            return -1;
        }

        coreaudio->mRecordSliceIndex = (coreaudio->mRecordSliceIndex + 1) % 4;
    }

    return 0;
}

FMOD_RESULT OutputCoreAudio::enumerate() {
    FMOD_RESULT result = FMOD_OK;
    char devicename[512];
    UInt32 devicelistsize = 0;
    AudioDeviceID* devicelist = nullptr;
    AudioBufferList* bufferlist = nullptr;

    if (!this->mInitialized) {
        Plugin::init();
    }

    for (int32_t i = 0; i < this->mNumOutputDevices; i++) {
        if (this->mOutputDevices[i].name) {
            FMOD_Memory_Free(this->mOutputDevices[i].name);
            this->mOutputDevices[i].name = nullptr;
        }
    }

    for (int32_t i = 0; i < this->mNumInputDevices; i++) {
        if (this->mInputDevices[i].name) {
            FMOD_Memory_Free(this->mInputDevices[i].name);
            this->mInputDevices[i].name = nullptr;
        }
    }

    this->mNumInputDevices = 0;
    this->mNumOutputDevices = 0;

    result = this->enumerateDefaultDevices();
    if (result != FMOD_OK) {
        return result;
    }

    if (AudioHardwareGetPropertyInfo(
            kAudioHardwarePropertyDevices,
            &devicelistsize,
            nullptr)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    auto numdevices = devicelistsize / sizeof(AudioDeviceID);

    if (numdevices) {
        devicelist = static_cast<AudioDeviceID*>(FMOD_Memory_Calloc(devicelistsize));
        if (!devicelist) {
            return FMOD_ERR_MEMORY;
        }

        if (AudioHardwareGetProperty(
                kAudioHardwarePropertyDevices,
                &devicelistsize,
                devicelist)) {
            result = FMOD_ERR_OUTPUT_INIT;
            goto cleanup;
        }

        for (int32_t i = 0; i < numdevices; i++) {
            UInt32 devicenamesize = 512;
            memset(devicename, 0, 512);

            if (AudioDeviceGetProperty(
                    devicelist[i],
                    0,
                    0,
                    kAudioDevicePropertyDeviceName,
                    &devicenamesize,
                    devicename)) {
                result = FMOD_ERR_OUTPUT_INIT;
                goto cleanup;
            }

            for (int32_t isinput = 0; isinput < 2; isinput++) {
                UInt32 bufferlistsize = 0;
                int32_t totalchannels = 0;
                if (AudioDeviceGetPropertyInfo(
                        devicelist[i],
                        0,
                        isinput != 0,
                        kAudioDevicePropertyStreamConfiguration,
                        &bufferlistsize,
                        nullptr)) {
                    result = FMOD_ERR_OUTPUT_INIT;
                    goto cleanup;
                }

                auto bufferlist = static_cast<AudioBufferList*>(FMOD_Memory_Calloc(bufferlistsize));
                if (!bufferlist) {
                    result = FMOD_ERR_MEMORY;
                    goto cleanup;
                }

                if (AudioDeviceGetProperty(
                        devicelist[i],
                        0,
                        isinput != 0,
                        kAudioDevicePropertyStreamConfiguration,
                        &bufferlistsize,
                        bufferlist)) {
                    result = FMOD_ERR_OUTPUT_INIT;
                    goto cleanup;
                }

                for (UInt32 k = 0; k < bufferlist->mNumberBuffers; k++) {
                    totalchannels += bufferlist->mBuffers[k].mNumberChannels;
                }

                if (totalchannels > 0) {
                    if (!isinput) {
                        if (this->mNumOutputDevices < FMOD_OUTPUT_MAXDRIVERS &&
                            FMOD_strcmp(devicename, this->mOutputDevices[0].name)) {

                            this->mOutputDevices[this->mNumOutputDevices].name = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(devicename) + 1));
                            if (!this->mOutputDevices[this->mNumOutputDevices].name) {
                                result = FMOD_ERR_MEMORY;
                                goto cleanup;
                            }

                            FMOD_strcpy(this->mOutputDevices[this->mNumOutputDevices].name, devicename);
                            this->mOutputDevices[mNumOutputDevices].id = devicelist[i];
                            this->mNumOutputDevices++;
                        }
                    } else {
                        if (this->mNumInputDevices < FMOD_OUTPUT_MAXDRIVERS &&
                            FMOD_strcmp(devicename, this->mInputDevices[0].name)) {
                            this->mInputDevices[mNumInputDevices].name = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(devicename) + 1));
                            if (!this->mInputDevices[mNumInputDevices].name) {
                                result = FMOD_ERR_MEMORY;
                                goto cleanup;
                            }

                            FMOD_strcpy(this->mInputDevices[mNumInputDevices].name, devicename);
                            this->mInputDevices[this->mNumInputDevices].id = devicelist[i];
                            this->mNumInputDevices++;
                        }
                    }
                }

                FMOD_Memory_Free(bufferlist);
                bufferlist = nullptr;
            }
        }
    }

cleanup:
    if (bufferlist) {
        FMOD_Memory_Free(bufferlist);
    }

    if (devicelist) {
        FMOD_Memory_Free(devicelist);
    }

    return result;
}

FMOD_RESULT OutputCoreAudio::enumerateDefaultDevices() {
    char outputname[512];
    char inputname[512];
    UInt32 outputnamesize = 512;
    UInt32 inputnamesize = 512;
    UInt32 outputidsize = sizeof(AudioDeviceID);
    UInt32 inputidsize = sizeof(AudioDeviceID);
    AudioDeviceID outputid = 0;
    AudioDeviceID inputid = 0;

    FMOD_memset(outputname, 0, 512);
    FMOD_memset(inputname, 0, 512);

    if (AudioHardwareGetProperty(
            kAudioHardwarePropertyDefaultOutputDevice,
            &outputidsize,
            &outputid)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (outputid) {
        if (AudioDeviceGetProperty(
                outputid,
                0,
                0,
                kAudioDevicePropertyDeviceName,
                &outputnamesize,
                &outputname)) {
            return FMOD_ERR_OUTPUT_INIT;
        }

        this->mOutputDevices[0].name = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(outputname) + 1));
        if (!this->mOutputDevices[0].name) {
            return FMOD_ERR_MEMORY;
        }

        FMOD_strcpy(this->mOutputDevices[0].name, outputname);
        this->mOutputDevices[0].id = outputid;
        this->mNumOutputDevices++;
    }

    if (AudioHardwareGetProperty(
            kAudioHardwarePropertyDefaultInputDevice,
            &inputidsize,
            &inputid)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (inputid) {
        if (AudioDeviceGetProperty(
                inputid,
                0,
                0,
                kAudioDevicePropertyDeviceName,
                &inputnamesize,
                &inputname)) {
            return FMOD_ERR_OUTPUT_INIT;
        }
        this->mInputDevices[0].name = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(inputname) + 1));
        if (!this->mInputDevices[0].name) {
            return FMOD_ERR_MEMORY;
        }

        FMOD_strcpy(this->mInputDevices[0].name, inputname);
        this->mInputDevices[0].id = inputid;
        this->mNumInputDevices++;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::getNumDrivers(int32_t* numdrivers) {
    FMOD_RESULT result;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    *numdrivers = this->mNumOutputDevices;
    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::getDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;

    if (!name || namelen < 1) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (id >= this->mNumOutputDevices) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(name, this->mOutputDevices[id].name, namelen - 1);

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::getDriverCapsEx(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode) {
    FMOD_RESULT result;
    UInt32 layoutsize = 0;
    UInt32 sampleratesize = sizeof(Float64);
    Float64 samplerate = 0.0;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0 || id >= this->mNumOutputDevices) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (AudioDeviceGetPropertyInfo(
            this->mOutputDevices[id].id,
            0,
            0,
            kAudioDevicePropertyPreferredChannelLayout,
            &layoutsize,
            nullptr)) {
        layoutsize = 0;
        *controlpanelspeakermode = FMOD_SPEAKERMODE_STEREO;
    } else {
        if (layoutsize) {
            auto layout = static_cast<AudioChannelLayout*>(FMOD_Memory_Alloc(layoutsize));
            if (!layout) {
                return FMOD_ERR_MEMORY;
            }

            if (AudioDeviceGetProperty(
                    this->mOutputDevices[id].id,
                    0,
                    0,
                    kAudioDevicePropertyPreferredChannelLayout,
                    &layoutsize,
                    layout)) {
                FMOD_Memory_Free(layout);
                return FMOD_ERR_OUTPUT_INIT;
            }

            int32_t numchannels = 0;
            if (layout->mNumberChannelDescriptions) {
                for (UInt32 i = 0; i < layout->mNumberChannelDescriptions; i++) {
                    if (layout->mChannelDescriptions[i].mChannelLabel != kAudioChannelLabel_Unknown) {
                        numchannels++;
                    }
                }
            }

            FMOD_Memory_Free(layout);

            switch (numchannels) {
            case 1:
                *controlpanelspeakermode = FMOD_SPEAKERMODE_MONO;
                break;
            case 4:
                *controlpanelspeakermode = FMOD_SPEAKERMODE_QUAD;
                break;
            case 6:
                *controlpanelspeakermode = FMOD_SPEAKERMODE_5POINT1;
                break;
            case 8:
                *controlpanelspeakermode = FMOD_SPEAKERMODE_7POINT1;
                break;
            default:
                *controlpanelspeakermode = FMOD_SPEAKERMODE_STEREO;
                break;
            }
        }
    }

    if (AudioDeviceGetProperty(
            this->mOutputDevices[id].id,
            0,
            0,
            kAudioDevicePropertyNominalSampleRate,
            &sampleratesize,
            &samplerate)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    *minfrequency = static_cast<int32_t>(samplerate);
    *maxfrequency = static_cast<int32_t>(samplerate);

    *caps = (FMOD_CAPS_OUTPUT_MULTICHANNEL |
             FMOD_CAPS_OUTPUT_FORMAT_PCM8 |
             FMOD_CAPS_OUTPUT_FORMAT_PCM16 |
             FMOD_CAPS_OUTPUT_FORMAT_PCM24 |
             FMOD_CAPS_OUTPUT_FORMAT_PCM32 |
             FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    FMOD_RESULT result;
    AudioComponentDescription desc;
    AudioComponent comp;
    UInt32 enableio;
    UInt32 basicdescsize = sizeof(AudioStreamBasicDescription);
    AudioStreamBasicDescription deviceformat;
    AudioStreamBasicDescription clientformat;
    AURenderCallbackStruct callback;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mNumOutputDevices <= 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioComponentInstanceNew(comp, &this->mOutputUnit)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    enableio = 1;

    if (AudioUnitSetProperty(
            this->mOutputUnit,
            kAudioOutputUnitProperty_EnableIO,
            kAudioUnitScope_Output,
            0,
            &enableio, sizeof(enableio))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    enableio = 0;
    if (AudioUnitSetProperty(
            this->mOutputUnit,
            kAudioOutputUnitProperty_EnableIO,
            kAudioUnitScope_Input,
            1,
            &enableio, sizeof(enableio))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioUnitSetProperty(
            this->mOutputUnit,
            kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global,
            0,
            &this->mOutputDevices[selecteddriver].id,
            sizeof(AudioDeviceID))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioUnitGetProperty(
            this->mOutputUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Output,
            0, &deviceformat, &basicdescsize)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    *outputformat = FMOD_SOUND_FORMAT_PCMFLOAT;
    *outputrate = static_cast<int32_t>(deviceformat.mSampleRate);

    FMOD_memset(&clientformat, 0, sizeof(clientformat));

    clientformat.mSampleRate = static_cast<Float64>(static_cast<int32_t>(deviceformat.mSampleRate));
    clientformat.mFormatID = kAudioFormatLinearPCM;
    clientformat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    clientformat.mFramesPerPacket = 1;
    clientformat.mChannelsPerFrame = outputchannels;
    clientformat.mBitsPerChannel = 32;
    clientformat.mBytesPerFrame = outputchannels * sizeof(float);
    clientformat.mBytesPerPacket = clientformat.mBytesPerFrame;

    if (AudioUnitSetProperty(
            this->mOutputUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            0,
            &clientformat,
            sizeof(clientformat))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    callback.inputProc = OutputCoreAudio::renderProc;
    callback.inputProcRefCon = this;

    if (AudioUnitSetProperty(
            this->mOutputUnit,
            kAudioUnitProperty_SetRenderCallback,
            kAudioUnitScope_Input,
            0,
            &callback,
            sizeof(callback))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioUnitInitialize(this->mOutputUnit)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::close() {
    AudioOutputUnitStop(this->mOutputUnit);
    AudioUnitUninitialize(this->mOutputUnit);

    AudioComponentInstanceDispose(this->mOutputUnit);

    for (int32_t i = 0; i < this->mNumOutputDevices; i++) {
        if (this->mOutputDevices[i].name) {
            FMOD_Memory_Free(this->mOutputDevices[i].name);
            this->mOutputDevices[i].name = nullptr;
        }
    }

    for (int32_t i = 0; i < this->mNumInputDevices; i++) {
        if (this->mInputDevices[i].name) {
            FMOD_Memory_Free(this->mInputDevices[i].name);
            this->mInputDevices[i].name = nullptr;
        }
    }

    this->mNumInputDevices = 0;
    this->mNumOutputDevices = 0;

    this->mInitialized = false;
    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::start() {
    auto ret = AudioOutputUnitStart(this->mOutputUnit);
    return ret ? FMOD_ERR_OUTPUT_DRIVERCALL : FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::stop() {
    auto ret = AudioOutputUnitStop(this->mOutputUnit);
    return ret ? FMOD_ERR_OUTPUT_DRIVERCALL : FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::getHandle(void** handle) {
    *handle = this->mOutputUnit;
    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::recordGetNumDrivers(int32_t* numdrivers) {
    FMOD_RESULT result;

    if (!numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    *numdrivers = this->mNumInputDevices;

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::recordGetDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;

    if (!name || namelen < 1) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (id >= this->mNumInputDevices) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(name, this->mInputDevices[id].name, namelen - 1);
    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::recordGetDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency) {
    FMOD_RESULT result;
    AudioValueRange* ranges;
    UInt32 datasize = 0;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0 || id >= this->mNumInputDevices) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (AudioDeviceGetPropertyInfo(
            this->mInputDevices[id].id,
            0,
            1,
            kAudioDevicePropertyAvailableNominalSampleRates,
            &datasize,
            nullptr)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (datasize) {
        ranges = static_cast<AudioValueRange*>(FMOD_Memory_Calloc(datasize));
        if (!ranges) {
            return FMOD_ERR_MEMORY;
        }

        if (AudioDeviceGetProperty(
                this->mInputDevices[id].id,
                0,
                1,
                kAudioDevicePropertyAvailableNominalSampleRates,
                &datasize,
                ranges)) {
            FMOD_Memory_Free(ranges);
            return FMOD_ERR_OUTPUT_INIT;
        }

        *minfrequency = static_cast<int32_t>(ranges[0].mMinimum);
        *maxfrequency = static_cast<int32_t>(ranges[0].mMaximum);

        auto numranges = datasize >> 4;
        for (UInt32 i = 1; i < numranges; i++) {
            if (ranges[i].mMinimum < static_cast<Float64>(*minfrequency)) {
                *minfrequency = static_cast<int32_t>(ranges[i].mMinimum);
            }
            if (static_cast<Float64>(*maxfrequency) < ranges[i].mMaximum) {
                *maxfrequency = static_cast<int32_t>(ranges[i].mMaximum);
            }
        }

        FMOD_Memory_Free(ranges);
    }

    *caps = (FMOD_CAPS_OUTPUT_MULTICHANNEL |
             FMOD_CAPS_OUTPUT_FORMAT_PCM8 |
             FMOD_CAPS_OUTPUT_FORMAT_PCM16 |
             FMOD_CAPS_OUTPUT_FORMAT_PCM24 |
             FMOD_CAPS_OUTPUT_FORMAT_PCM32 |
             FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT);

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::recordStart(int32_t id, FMOD_SOUND* sound, int32_t loop) {
    FMOD_RESULT result;
    AudioComponentDescription desc;
    AudioComponent comp;
    UInt32 enableio;
    UInt32 basicdescsize = sizeof(AudioStreamBasicDescription);
    UInt32 framelensize = sizeof(UInt32);
    UInt32 wantedframes = 0x400;
    AudioStreamBasicDescription deviceformat;
    AudioStreamBasicDescription clientformat;
    AURenderCallbackStruct callback;
    OSStatus status;
    auto soundi = reinterpret_cast<SoundI*>(sound);

    if (!soundi || !soundi->mLength) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mNumInputDevices <= 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioComponentInstanceNew(comp, &this->mInputUnit)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    enableio = 1;
    if (AudioUnitSetProperty(
            this->mInputUnit,
            kAudioOutputUnitProperty_EnableIO,
            kAudioUnitScope_Input,
            1,
            &enableio,
            sizeof(enableio))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    enableio = 0;
    if (AudioUnitSetProperty(
            this->mInputUnit,
            kAudioOutputUnitProperty_EnableIO,
            kAudioUnitScope_Output,
            0,
            &enableio,
            sizeof(enableio))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioUnitSetProperty(
            this->mInputUnit,
            kAudioOutputUnitProperty_CurrentDevice,
            kAudioUnitScope_Global,
            0,
            &this->mInputDevices[id].id,
            sizeof(AudioDeviceID))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioUnitGetProperty(
            this->mInputUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            1,
            &deviceformat,
            &basicdescsize) != noErr) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    this->mRecordFormat = FMOD_SOUND_FORMAT_PCMFLOAT;
    this->mRecordRate = static_cast<int32_t>(deviceformat.mSampleRate);

    clientformat.mSampleRate = deviceformat.mSampleRate;
    clientformat.mFormatID = kAudioFormatLinearPCM;
    clientformat.mFormatFlags = kAudioFormatFlagIsFloat |
                                kAudioFormatFlagIsPacked;
    clientformat.mBytesPerPacket = soundi->mChannels / 4;
    clientformat.mFramesPerPacket = 1;
    clientformat.mBytesPerFrame = clientformat.mBytesPerPacket;
    clientformat.mChannelsPerFrame = soundi->mChannels;
    clientformat.mBitsPerChannel = 32;

    this->mRecordNumBytesPerFrame = clientformat.mBytesPerFrame;

    if (AudioUnitSetProperty(
            this->mInputUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Output,
            1,
            &clientformat,
            sizeof(clientformat))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioUnitGetProperty(
            this->mInputUnit,
            kAudioUnitProperty_MaximumFramesPerSlice,
            kAudioUnitScope_Global,
            0,
            &this->mRecordMaxFramesPerSlice,
            &framelensize)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (this->mRecordMaxFramesPerSlice < 0x400) {
        if (!AudioDeviceSetProperty(
                this->mInputDevices[id].id,
                0,
                0,
                1,
                kAudioDevicePropertyBufferFrameSize,
                sizeof(UInt32),
                &wantedframes) &&
            !AudioUnitSetProperty(
                this->mInputUnit,
                kAudioUnitProperty_MaximumFramesPerSlice,
                kAudioUnitScope_Global, 0,
                &wantedframes,
                sizeof(UInt32))) {
            this->mRecordMaxFramesPerSlice = wantedframes;
        }
    }

    this->mRecordBufferLength = this->mRecordMaxFramesPerSlice * 4;
    this->mRecordBuffer = FMOD_Memory_Calloc(this->mRecordBufferLength * this->mRecordNumBytesPerFrame);
    if (!this->mRecordBuffer) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    this->mRecordBufferList.mNumberBuffers = 1;
    this->mRecordBufferList.mBuffers[0].mNumberChannels = clientformat.mChannelsPerFrame;
    this->mRecordBufferList.mBuffers[0].mDataByteSize = this->mRecordNumBytesPerFrame * this->mRecordMaxFramesPerSlice;
    this->mRecordBufferList.mBuffers[0].mData = this->mRecordBuffer;
    this->mRecordSliceIndex = 0;

    callback.inputProc = OutputCoreAudio::recordProc;
    callback.inputProcRefCon = this;

    if (AudioUnitSetProperty(
            this->mInputUnit,
            kAudioOutputUnitProperty_SetInputCallback,
            kAudioUnitScope_Global,
            0,
            &callback,
            sizeof(callback))) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    status = AudioUnitInitialize(this->mInputUnit);
    if (status == kAudioUnitErr_FormatNotSupported) {
        return FMOD_ERR_FORMAT;
    } else if (status) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    if (AudioOutputUnitStart(this->mInputUnit)) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::recordStop() {
    if (this->mInputUnit) {
        AudioOutputUnitStop(this->mInputUnit);
        AudioUnitUninitialize(this->mInputUnit);

        AudioComponentInstanceDispose(this->mInputUnit);

        if (this->mRecordBuffer) {
            FMOD_Memory_Free(this->mRecordBuffer);
            this->mRecordBuffer = nullptr;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::recordGetPosition(uint32_t* pcm) {
    *pcm = this->mRecordMaxFramesPerSlice * this->mRecordSliceIndex;
    return FMOD_OK;
}

FMOD_RESULT OutputCoreAudio::recordLock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    uint32_t lenbytes = this->mRecordNumBytesPerFrame * this->mRecordBufferLength;

    if (offset >= lenbytes) {
        *ptr1 = nullptr;
        *ptr2 = nullptr;
        *len1 = 0;
        *len2 = 0;
        return FMOD_ERR_INVALID_PARAM;
    }

    if (lenbytes < length) {
        length = lenbytes;
    }

    if (lenbytes < length + offset) {
        *ptr1 = static_cast<uint8_t*>(this->mRecordBuffer) + offset;
        *len1 = lenbytes - offset;
        *ptr2 = this->mRecordBuffer;
        *len2 = length - (lenbytes - offset);
    } else {
        *ptr1 = static_cast<uint8_t*>(this->mRecordBuffer) + offset;
        *len1 = length;
        *ptr2 = nullptr;
        *len2 = 0;
    }
    return FMOD_OK;
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::getDriverCapsExCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->getDriverCapsEx(id, caps, minfrequency, maxfrequency, controlpanelspeakermode);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->close();
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::startCallback(FMOD_OUTPUT_STATE* output) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->start();
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::stopCallback(FMOD_OUTPUT_STATE* output) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->stop();
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->getHandle(handle);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::recordGetNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->recordGetNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::recordGetDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->recordGetDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::recordGetDriverCapsCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->recordGetDriverCaps(id, caps, minfrequency, maxfrequency);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::recordStartCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_SOUND* sound, int32_t loop) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->recordStart(id, sound, loop);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::recordStopCallback(FMOD_OUTPUT_STATE* output) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->recordStop();
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::recordGetPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->recordGetPosition(pcm);
}

FMOD_RESULT F_CALLBACK OutputCoreAudio::recordLockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    auto coreaudio = static_cast<OutputCoreAudio*>(output);
    return coreaudio->recordLock(offset, length, ptr1, ptr2, len1, len2);
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputCoreAudio::getDescriptionEx() {
    FMOD_memset(&coreaudiooutput, 0, sizeof(FMOD_OUTPUT_DESCRIPTION_EX));

    coreaudiooutput.name = "FMOD Core Audio Output";
    coreaudiooutput.version = 0x10200;
    coreaudiooutput.polling = 0;

    coreaudiooutput.getnumdrivers = OutputCoreAudio::getNumDriversCallback;
    coreaudiooutput.getdrivername = OutputCoreAudio::getDriverNameCallback;
    coreaudiooutput.getdrivercapsex = OutputCoreAudio::getDriverCapsExCallback;
    coreaudiooutput.init = OutputCoreAudio::initCallback;
    coreaudiooutput.close = OutputCoreAudio::closeCallback;
    coreaudiooutput.start = OutputCoreAudio::startCallback;
    coreaudiooutput.stop = OutputCoreAudio::stopCallback;
    coreaudiooutput.gethandle = OutputCoreAudio::getHandleCallback;

    coreaudiooutput.record_getnumdrivers = OutputCoreAudio::recordGetNumDriversCallback;
    coreaudiooutput.record_getdriverinfo = OutputCoreAudio::recordGetDriverNameCallback;
    coreaudiooutput.record_getdrivercaps = OutputCoreAudio::recordGetDriverCapsCallback;
    coreaudiooutput.record_start = OutputCoreAudio::recordStartCallback;
    coreaudiooutput.record_stop = OutputCoreAudio::recordStopCallback;
    coreaudiooutput.record_getposition = OutputCoreAudio::recordGetPositionCallback;
    coreaudiooutput.record_lock = OutputCoreAudio::recordLockCallback;

    coreaudiooutput.mType = FMOD_OUTPUTTYPE_COREAUDIO;
    coreaudiooutput.mSize = sizeof(OutputCoreAudio);

    return &coreaudiooutput;
}

} // namespace FMOD
