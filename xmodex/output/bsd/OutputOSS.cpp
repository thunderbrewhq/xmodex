#include "xmodex/output/bsd/OutputOSS.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/output/Output.hpp"
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#define RECORD_MAXBLOCKS 100

namespace FMOD {

FMOD_OUTPUT_DESCRIPTION_EX ossoutput;

void closeDevice(int32_t fd) {
    ioctl(fd, SNDCTL_DSP_RESET, 0);
    close(fd);
}

int32_t OutputOSS::calcHigherLogTwo(int32_t v) {
    int32_t ret = 0;
    while ((1 << ret) < v) {
        ret++;
    }
    return ret;
}

void F_CALLBACK OutputOSS::mixThreadCallback(void* data) {
    auto oss = static_cast<OutputOSS*>(data);
    oss->updateMixer();
}

void F_CALLBACK OutputOSS::recordThreadCallback(void* data) {
    auto oss = static_cast<OutputOSS*>(data);
    oss->updateRecord();
}

FMOD_RESULT OutputOSS::setDeviceParams(int32_t fd, int32_t bits, int32_t channels, int32_t samplerate, int32_t buffersize) {
    int32_t p_format = AFMT_S16_LE;
    int32_t p_samplerate = samplerate;
    int32_t p_channels = channels;
    int32_t p_fragment = 0x20000 | OutputOSS::calcHigherLogTwo(((buffersize / 1024) << 10) / 2);

    this->mFragment = p_fragment;
    ioctl(this->mHandle, SNDCTL_DSP_SETFRAGMENT, &p_fragment);

    if (ioctl(fd, SNDCTL_DSP_SETFMT, &p_format) < 0) {
        return FMOD_ERR_OUTPUT_FORMAT;
    }

    if (p_format != AFMT_S16_LE) {
        return FMOD_ERR_OUTPUT_FORMAT;
    }

    if (ioctl(fd, SNDCTL_DSP_CHANNELS, &p_channels) < 0) {
        return FMOD_ERR_OUTPUT_FORMAT;
    }

    if (p_channels != channels) {
        return FMOD_ERR_OUTPUT_FORMAT;
    }

    if (ioctl(fd, SNDCTL_DSP_SPEED, &p_samplerate) < 0) {
        return FMOD_ERR_OUTPUT_FORMAT;
    }

    if (p_samplerate != samplerate) {
        return FMOD_ERR_OUTPUT_FORMAT;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputOSS::enumerate() {
    if (this->mEnumerated) {
        return FMOD_OK;
    }

    Plugin::init();

    this->mNumDrivers = 0;

    auto dir = opendir("/dev");
    if (!dir) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    dirent* ent;

    while ((ent = readdir(dir))) {
        if (!FMOD_strncmp(ent->d_name, "dsp", 3)) {
            bool valid = true;
            for (auto p = ent->d_name + 3; *p; p++) {
                if (!FMOD_isdigit(static_cast<uint8_t>(*p))) {
                    valid = false;
                    break;
                }
            }
            if (!valid) {
                continue;
            }

            this->mDriverNames[this->mNumDrivers] = static_cast<char*>(FMOD_Memory_Calloc(FMOD_strlen(ent->d_name) + 7));
            if (!this->mDriverNames[this->mNumDrivers]) {
                return FMOD_ERR_MEMORY;
            }

            sprintf(this->mDriverNames[this->mNumDrivers], "/dev/%s", ent->d_name);
            if (!FMOD_strcmp(this->mDriverNames[this->mNumDrivers], "/dev/dsp")) {
                FMOD_strcpy(this->mDriverNames[this->mNumDrivers], this->mDriverNames[0]);
                FMOD_strcpy(this->mDriverNames[0], "/dev/dsp");
            }

            this->mNumDrivers++;
        }
    }

    closedir(dir);
    this->mEnumerated = true;
    return FMOD_OK;
}

FMOD_RESULT OutputOSS::updateMixer() {
    uint32_t blocksize;
    uint32_t bytes;
    this->mSystem->getDSPBufferSize(&blocksize, nullptr);
    SoundI::getBytesFromSamples(blocksize, &bytes, this->mNumOutputChannels, this->mOutputFormat);
    this->mSystem->mDSPTimeStamp.stampIn();
    Output::mix(this->mOutputBuffer, blocksize);
    this->mSystem->mDSPTimeStamp.stampOut(95);

    write(this->mHandle, this->mOutputBuffer, bytes);

    return FMOD_OK;
}

FMOD_RESULT OutputOSS::updateRecord() {
    if (!this->mRecording) {
        return FMOD_OK;
    }

    auto ret = read(
        this->mHandle,
        static_cast<uint8_t*>(this->mRecordBuffer) + (this->mRecordBlockLenBytes * this->mRecordCurrentBlock),
        this->mRecordBlockLenBytes);

    this->mRecordCurrentBlock++;
    if (this->mRecordCurrentBlock >= 100) {
        this->mRecordCurrentBlock = 0;
    }

    this->mRecordPosition += ret;
    if (this->mRecordBufferBytes <= 0) {
        this->mRecordPosition = 0;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputOSS::getNumDrivers(int32_t* numdrivers) {
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

FMOD_RESULT OutputOSS::getDriverName(int32_t id, char* name, int32_t namelen) {
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

FMOD_RESULT OutputOSS::init(int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    FMOD_RESULT result;

    this->mNumOutputChannels = outputchannels;
    this->mOutputFormat = *outputformat;
    this->mSelectedDriver = selecteddriver;
    this->mOutputRate = *outputrate;

    result = this->enumerate();
    if (result != FMOD_OK) {
        return result;
    }

    if (!this->mNumDrivers) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    const char* device;

    if (selecteddriver < 0) {
        this->mHandle = open("/dev/dsp", O_RDWR | O_NONBLOCK);
        closeDevice(this->mHandle);
        device = "/dev/dsp";
    } else {
        this->mHandle = open(this->mDriverNames[selecteddriver], O_RDWR | O_NONBLOCK);
        if (this->mHandle < 0) {
            return FMOD_ERR_OUTPUT_INIT;
        }
        closeDevice(this->mHandle);
        device = this->mDriverNames[selecteddriver];
    }

    this->mHandle = open(device, O_RDWR);
    if (this->mHandle < 0) {
        return FMOD_ERR_OUTPUT_INIT;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputOSS::close() {
    if (this->mHandle != -1) {
        closeDevice(this->mHandle);
    }

    this->mHandle = -1;

    while (this->mNumDrivers > 0) {
        this->mNumDrivers--;
        FMOD_Memory_Free(this->mDriverNames[this->mNumDrivers]);
        this->mDriverNames[this->mNumDrivers] = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputOSS::start() {
    FMOD_RESULT result;
    uint32_t bufferlength;
    uint32_t bytes;
    int32_t bits;

    result = this->mSystem->getDSPBufferSize(&bufferlength, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = SoundI::getBytesFromSamples(
        bufferlength,
        &bytes,
        this->mNumOutputChannels,
        this->mOutputFormat);

    if (result != FMOD_OK) {
        return result;
    }

    result = SoundI::getBitsFromFormat(this->mOutputFormat, &bits);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->setDeviceParams(
        this->mHandle,
        bits,
        this->mNumOutputChannels,
        this->mOutputRate,
        bytes);
    if (result != FMOD_OK) {
        return result;
    }

    this->mOutputBuffer = FMOD_Memory_Alloc(bytes);
    if (!this->mOutputBuffer) {
        return FMOD_ERR_MEMORY;
    }

    return this->mMixerThread.initThread(
        "OSS Mixer",
        OutputOSS::mixThreadCallback,
        this,
        Thread::PRIORITY_HIGH,
        nullptr,
        0,
        false,
        0);
}

FMOD_RESULT OutputOSS::stop() {
    this->mMixerThread.closeThread();

    ioctl(this->mHandle, SNDCTL_DSP_RESET, 0);

    if (this->mOutputBuffer) {
        FMOD_Memory_Free(this->mOutputBuffer);
        this->mOutputBuffer = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT OutputOSS::getHandle(void** handle) {
    *handle = reinterpret_cast<void*>(this->mHandle);
    return FMOD_OK;
}

FMOD_RESULT OutputOSS::recordGetNumDrivers(int32_t* numdrivers) {
    return this->getNumDrivers(numdrivers);
}

FMOD_RESULT OutputOSS::recordGetDriverName(int32_t id, char* name, int32_t namelen) {
    return this->getDriverName(id, name, namelen);
}

FMOD_RESULT OutputOSS::recordStart(int32_t id, FMOD_SOUND* sound, int32_t loop) {
    FMOD_RESULT result;

    auto soundi = reinterpret_cast<SoundI*>(sound);
    if (!sound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mRecordPosition = 0;
    this->mRecordCurrentBlock = 0;

    auto format = soundi->mFormat;
    auto channels = soundi->mChannels;
    auto frequency = soundi->mDefaultFrequency;

    this->mRecordFormat = format;
    this->mRecordChannels = channels;

    int32_t bits;
    uint32_t buffersize;

    SoundI::getBitsFromFormat(format, &bits);
    buffersize = (this->mRecordChannels * bits) / 8;
    this->mRecordFrameLenBytes = buffersize;
    buffersize *= (static_cast<int32_t>(frequency) / (RECORD_MAXBLOCKS * 2));
    this->mRecordBlockLenBytes = buffersize;

    result = this->setDeviceParams(
        this->mHandle,
        bits,
        this->mRecordChannels,
        static_cast<int32_t>(frequency),
        buffersize);
    if (result != FMOD_OK) {
        return result;
    }

    this->mRecordBufferBytes = this->mRecordBlockLenBytes * RECORD_MAXBLOCKS;
    this->mRecordBuffer = FMOD_Memory_Alloc(this->mRecordBufferBytes);
    if (!this->mRecordBuffer) {
        return FMOD_ERR_MEMORY;
    }

    SoundI::getSamplesFromBytes(
        this->mRecordBufferBytes,
        &this->mRecordBufferLength,
        this->mRecordChannels,
        this->mRecordFormat);

    return this->mRecordThread.initThread(
        "OSS Record",
        OutputOSS::recordThreadCallback,
        this,
        Thread::PRIORITY_NORMAL,
        nullptr,
        0,
        false,
        0);
}

FMOD_RESULT OutputOSS::recordStop() {
    FMOD_RESULT result;
    uint32_t bufferlength;
    uint32_t buffersize;
    int32_t bits;

    this->mRecordThread.closeThread();

    if (this->mRecordBuffer) {
        FMOD_Memory_Free(this->mRecordBuffer);
        this->mRecordBuffer = nullptr;
    }

    result = this->mSystem->getDSPBufferSize(&bufferlength, nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = SoundI::getBytesFromSamples(bufferlength, &buffersize, this->mNumOutputChannels, this->mOutputFormat);
    if (result != FMOD_OK) {
        return result;
    }

    result = SoundI::getBitsFromFormat(this->mOutputFormat, &bits);
    if (result != FMOD_OK) {
        return result;
    }

    return this->setDeviceParams(
        this->mHandle,
        bits,
        this->mNumOutputChannels,
        this->mOutputRate,
        buffersize);
}

FMOD_RESULT OutputOSS::recordGetPosition(uint32_t* pcm) {
    SoundI::getSamplesFromBytes(
        this->mRecordPosition / this->mRecordFrameLenBytes,
        pcm,
        this->mRecordChannels,
        this->mRecordFormat);
    return FMOD_OK;
}

FMOD_RESULT OutputOSS::recordLock(uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
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

FMOD_RESULT F_CALLBACK OutputOSS::getNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->getNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputOSS::getDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->getDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputOSS::initCallback(FMOD_OUTPUT_STATE* output, int32_t selecteddriver, FMOD_INITFLAGS flags, int32_t* outputrate, int32_t outputchannels, FMOD_SOUND_FORMAT* outputformat, int32_t dspbufferlength, int32_t dspnumbuffers, void* extradriverdata) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->init(selecteddriver, flags, outputrate, outputchannels, outputformat, dspbufferlength, dspnumbuffers, extradriverdata);
}

FMOD_RESULT F_CALLBACK OutputOSS::closeCallback(FMOD_OUTPUT_STATE* output) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->close();
}

FMOD_RESULT F_CALLBACK OutputOSS::startCallback(FMOD_OUTPUT_STATE* output) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->start();
}

FMOD_RESULT F_CALLBACK OutputOSS::stopCallback(FMOD_OUTPUT_STATE* output) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->stop();
}

FMOD_RESULT F_CALLBACK OutputOSS::getHandleCallback(FMOD_OUTPUT_STATE* output, void** handle) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->getHandle(handle);
}

FMOD_RESULT F_CALLBACK OutputOSS::recordGetNumDriversCallback(FMOD_OUTPUT_STATE* output, int32_t* numdrivers) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->recordGetNumDrivers(numdrivers);
}

FMOD_RESULT F_CALLBACK OutputOSS::recordGetDriverNameCallback(FMOD_OUTPUT_STATE* output, int32_t id, char* name, int32_t namelen) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->recordGetDriverName(id, name, namelen);
}

FMOD_RESULT F_CALLBACK OutputOSS::recordStartCallback(FMOD_OUTPUT_STATE* output, int32_t id, FMOD_SOUND* sound, int32_t loop) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->recordStart(id, sound, loop);
}

FMOD_RESULT F_CALLBACK OutputOSS::recordStopCallback(FMOD_OUTPUT_STATE* output) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->recordStop();
}

FMOD_RESULT F_CALLBACK OutputOSS::recordGetPositionCallback(FMOD_OUTPUT_STATE* output, uint32_t* pcm) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->recordGetPosition(pcm);
}

FMOD_RESULT F_CALLBACK OutputOSS::recordLockCallback(FMOD_OUTPUT_STATE* output, uint32_t offset, uint32_t length, void** ptr1, void** ptr2, uint32_t* len1, uint32_t* len2) {
    auto oss = static_cast<OutputOSS*>(output);
    return oss->recordLock(offset, length, ptr1, ptr2, len1, len2);
}

FMOD_OUTPUT_DESCRIPTION_EX* OutputOSS::getDescriptionEx() {
    ossoutput.name = "FMOD OSS Output";
    ossoutput.version = 0x10100;
    ossoutput.polling = 0;

    ossoutput.initex = nullptr;
    ossoutput.createsample = nullptr;
    ossoutput.getnumdrivers = OutputOSS::getNumDriversCallback;
    ossoutput.getdrivername = OutputOSS::getDriverNameCallback;
    ossoutput.init = OutputOSS::initCallback;
    ossoutput.close = OutputOSS::closeCallback;
    ossoutput.start = OutputOSS::startCallback;
    ossoutput.stop = OutputOSS::stopCallback;
    ossoutput.gethandle = OutputOSS::getHandleCallback;

    ossoutput.record_getnumdrivers = OutputOSS::recordGetNumDriversCallback;
    ossoutput.record_getdriverinfo = OutputOSS::recordGetDriverNameCallback;
    ossoutput.record_start = OutputOSS::recordStartCallback;
    ossoutput.record_stop = OutputOSS::recordStopCallback;
    ossoutput.record_getposition = OutputOSS::recordGetPositionCallback;
    ossoutput.record_lock = OutputOSS::recordLockCallback;
    ossoutput.record_unlock = nullptr;

    ossoutput.mType = FMOD_OUTPUTTYPE_OSS;
    ossoutput.mSize = sizeof(OutputOSS);

    return &ossoutput;
}

} // namespace FMOD
