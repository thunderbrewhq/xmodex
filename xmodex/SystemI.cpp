#include "xmodex/SystemI.hpp"
#include "fmod.hpp"
#include "xmodex/AsyncData.hpp"
#include "xmodex/AsyncThread.hpp"
#include "xmodex/LocalMemoryCounter.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/PluginFactory.hpp"
#include "xmodex/ReverbI.hpp"
#include "xmodex/SpeakerMap.hpp"
#include "xmodex/channel/ChannelI.hpp"
#include "xmodex/channel/ChannelPool.hpp"
#include "xmodex/channel/ChannelReal.hpp"
#include "xmodex/channel/ChannelStream.hpp"
#include "xmodex/codec/CodecMPEG.hpp"
#include "xmodex/codec/CodecOggVorbis.hpp"
#include "xmodex/codec/CodecTag.hpp"
#include "xmodex/codec/CodecWav.hpp"
#include "xmodex/dsp/DSPChorus.hpp"
#include "xmodex/dsp/DSPCodec.hpp"
#include "xmodex/dsp/DSPCompressor.hpp"
#include "xmodex/dsp/DSPConnectionRequest.hpp"
#include "xmodex/dsp/DSPDistortion.hpp"
#include "xmodex/dsp/DSPEcho.hpp"
#include "xmodex/dsp/DSPFlange.hpp"
#include "xmodex/dsp/DSPHighPass.hpp"
#include "xmodex/dsp/DSPI.hpp"
#include "xmodex/dsp/DSPLowPass.hpp"
#include "xmodex/dsp/DSPLowPass2.hpp"
#include "xmodex/dsp/DSPLowPassSimple.hpp"
#include "xmodex/dsp/DSPNormalize.hpp"
#include "xmodex/dsp/DSPOscillator.hpp"
#include "xmodex/dsp/DSPParamEq.hpp"
#include "xmodex/dsp/DSPPitchShift.hpp"
#include "xmodex/dsp/DSPSfxReverb.hpp"
#include "xmodex/file/DiskFile.hpp"
#include "xmodex/file/File.hpp"
#include "xmodex/file/MemoryFile.hpp"
#include "xmodex/file/NullFile.hpp"
#include "xmodex/file/UserFile.hpp"
#include "xmodex/os/CDDA.hpp"
#include "xmodex/os/CriticalSection.hpp"
#include "xmodex/os/Driver.hpp"
#include "xmodex/os/Output.hpp"
#include "xmodex/os/Thread.hpp"
#include "xmodex/os/Time.hpp"
#include "xmodex/output/Output.hpp"
#include "xmodex/output/OutputEmulated.hpp"
#include "xmodex/output/OutputNoSound.hpp"
#include "xmodex/output/OutputNoSound_NRT.hpp"
#include "xmodex/output/OutputPolled.hpp"
#include "xmodex/output/OutputSoftware.hpp"
#include "xmodex/output/OutputWavWriter.hpp"
#include "xmodex/output/OutputWavWriter_NRT.hpp"
#include "xmodex/sound/Sample.hpp"
#include "xmodex/sound/SoundGroupI.hpp"
#include "xmodex/sound/Stream.hpp"
#include <cstddef>

namespace FMOD {

FMOD_OS_CRITICALSECTION* SystemI::gSoundListCrit = nullptr;

SystemI::SystemI() {
    this->mInitialized = false;
    this->mPluginsLoaded = false;
    this->mOutputType = FMOD_OUTPUTTYPE_AUTODETECT;
    this->mOutput = nullptr;
    this->mSoftware = nullptr;
    this->mEmulated = nullptr;
    this->mMainThreadID = 0;
    this->mChannel = nullptr;
    this->mPluginFactory = nullptr;
    this->mLastTimeStamp = 0;
    this->mStreamFileBufferSize = 16384;
    this->mStreamFileBufferSizeType = FMOD_TIMEUNIT_RAWBYTES;
    this->mUserData = nullptr;
    this->mNumSoftwareChannels = 64;
    this->mMinHardwareChannels2D = 0;
    this->mMaxHardwareChannels2D = 1000;
    this->mMinHardwareChannels3D = 0;
    this->mMaxHardwareChannels3D = 1000;
    this->mDSPBlockSize = 1024;
    this->mDSPBufferSize = 4096;
    this->mDSPTempBuff[0] = nullptr;
    this->mDSPTempBuff[1] = nullptr;
    this->mDSPTempBuffMem[0] = nullptr;
    this->mDSPTempBuffMem[1] = nullptr;
    this->mDSPTempBuffIndex = 0;

    FMOD_memset(this->mDSPMixBuff, 0, sizeof(this->mDSPMixBuff));

    this->mMaxInputChannels = 6;
    this->mMaxOutputChannels = 0;

    this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_LEFT, -1.0f, 1.0f, true);
    this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_RIGHT, 1.0f, 1.0f, true);
    this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_CENTER, 0.0f, 1.0f, true);
    this->set3DSpeakerPosition(FMOD_SPEAKER_LOW_FREQUENCY, 0.0f, 0.0f, true);
    this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_LEFT, -1.0f, -1.0f, true);
    this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_RIGHT, 1.0f, -1.0f, true);
    this->set3DSpeakerPosition(FMOD_SPEAKER_SIDE_LEFT, -1.0f, 0.0f, true);
    this->set3DSpeakerPosition(FMOD_SPEAKER_SIDE_RIGHT, 1.0f, 0.0f, true);

    this->mOutputFormat = FMOD_SOUND_FORMAT_PCM16;

    this->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);

    this->mOutputRate = 48000;
    this->mDistanceScale = 1.0f;
    this->mRolloffScale = 1.0f;
    this->mDopplerScale = 1.0f;
    this->mOutputHandle = 0;
    this->mSelectedDriver = 0;
    this->mReverb3DActive = false;
    this->mRecordID = 0;
    this->mResampleMethod = FMOD_DSP_RESAMPLER_LINEAR;
    this->mNumListeners = 1;

    FMOD_REVERB_PROPERTIES prop = FMOD_PRESET_OFF;
    this->setReverbAmbientProperties(&prop);

    // this->mGeometryMgr.mSystem = this;
    // this->mGeometryList = nullptr;

    FMOD_memset(this->mPluginPath, 0, FMOD_STRING_MAXPATHLEN);

    this->mAdvancedSettings.HRTFMinAngle = 180.0f;
    this->mAdvancedSettings.maxXMAcodecs = 0;
    this->mAdvancedSettings.maxADPCMcodecs = 0;
    this->mAdvancedSettings.HRTFMaxAngle = 360.0f;
    this->mAdvancedSettings.maxMPEGcodecs = 0;
    this->mAdvancedSettings.maxPCMcodecs = 0;
    this->mAdvancedSettings.HRTFFreq = 4000.0f;
    this->mAdvancedSettings.max3DReverbDSPs = 0;
    this->mAdvancedSettings.eventqueuesize = 32;
    this->mAdvancedSettings.vol0virtualvol = 0.0f;

    this->mMPEGPluginHandle = -1;
    this->mFSBPluginHandle = -1;
    this->mWAVPluginHandle = -1;

    this->mDSPCodecPool_MPEG.mSystem = this;
    this->mDSPCodecPool_ADPCM.mSystem = this;

    this->mASIOSpeakerList[0] = FMOD_SPEAKER_FRONT_LEFT;
    this->mASIOSpeakerList[1] = FMOD_SPEAKER_FRONT_RIGHT;
    this->mASIOSpeakerList[2] = FMOD_SPEAKER_FRONT_CENTER;
    this->mASIOSpeakerList[3] = FMOD_SPEAKER_LOW_FREQUENCY;
    this->mASIOSpeakerList[4] = FMOD_SPEAKER_BACK_LEFT;
    this->mASIOSpeakerList[5] = FMOD_SPEAKER_BACK_RIGHT;
    this->mASIOSpeakerList[6] = FMOD_SPEAKER_SIDE_LEFT;
    this->mASIOSpeakerList[7] = FMOD_SPEAKER_SIDE_RIGHT;
    this->mASIOSpeakerList[8] = FMOD_SPEAKER_FRONT_LEFT;
    this->mASIOSpeakerList[9] = FMOD_SPEAKER_FRONT_RIGHT;
    this->mASIOSpeakerList[10] = FMOD_SPEAKER_FRONT_LEFT;
    this->mASIOSpeakerList[11] = FMOD_SPEAKER_FRONT_RIGHT;
    this->mASIOSpeakerList[12] = FMOD_SPEAKER_FRONT_LEFT;
    this->mASIOSpeakerList[14] = FMOD_SPEAKER_FRONT_LEFT;
    this->mASIOSpeakerList[13] = FMOD_SPEAKER_FRONT_RIGHT;
    this->mASIOSpeakerList[15] = FMOD_SPEAKER_FRONT_RIGHT;
}

FMOD_RESULT SystemI::allocDSPCodec(FMOD_SOUND_FORMAT format, DSPI** dsp) {
    if (format == FMOD_SOUND_FORMAT_MPEG) {
        return this->mDSPCodecPool_MPEG.alloc(dsp);
    }
    if (format == FMOD_SOUND_FORMAT_IMAADPCM) {
        return this->mDSPCodecPool_ADPCM.alloc(dsp);
    }
    return FMOD_ERR_FORMAT;
}

FMOD_RESULT SystemI::checkDriverList(bool fromsystemupdate) {
    // XXX
    FMOD_RESULT result;
    bool devicelistchanged = false;

    if (fromsystemupdate && !this->mCallback[FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED]) {
        return FMOD_OK;
    }

    result = FMOD_OS_CheckDriverList(&devicelistchanged);
    if (result != FMOD_OK) {
        return result;
    }

    if (devicelistchanged && fromsystemupdate) {
        this->mCallback[FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED](
            reinterpret_cast<FMOD_SYSTEM*>(this),
            FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED,
            0,
            0);
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::close() {
    return this->closeEx(false);
}

FMOD_RESULT SystemI::closeEx(bool calledfrominit) {
    FMOD_RESULT result;
    Channel* channel;

    this->recordStop();

    if (!calledfrominit) {
        for (int32_t i = 0; i < this->mNumChannels; i++) {
            if (reinterpret_cast<System*>(this)->getChannel(i, &channel) == FMOD_OK) {
                channel->stop();
            }
        }

        this->update();

        if (this->mStreamThreadActive) {
            this->mStreamThread.closeThread();

            this->mStreamThreadActive = false;

            FMOD_OS_CriticalSection_Free(this->mStreamRealchanCrit);
            this->mStreamRealchanCrit = nullptr;

            FMOD_OS_CriticalSection_Free(this->mStreamUpdateCrit);
            this->mStreamUpdateCrit = nullptr;

            FMOD_OS_CriticalSection_Free(this->mStreamListCrit);
            this->mStreamListCrit = nullptr;
        }

        if (gGlobal->gSystemInitCount == 1 && this->mInitialized) {
            result = AsyncThread::shutDown();
            if (result != FMOD_OK) {
                return result;
            }

            if (gSoundListCrit) {
                FMOD_OS_CriticalSection_Free(gSoundListCrit);
                gSoundListCrit = nullptr;
            }

            result = File::shutDown();
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    if (this->mChannelGroup) {
        result = this->mChannelGroup->releaseInternal();
        if (result != FMOD_OK) {
            return result;
        }
        this->mChannelGroup = nullptr;
    }

    while (!this->mChannelGroupHead.isEmpty()) {
        result = reinterpret_cast<ChannelGroupI*>(this->mChannelGroupHead.getNext())->release();
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (this->mSoundGroup) {
        result = this->mSoundGroup->releaseInternal();
        if (result != FMOD_OK) {
            return result;
        }
        this->mSoundGroup = nullptr;
    }

    if (this->mOutput && !calledfrominit) {
        if (this->mOutput->mDescription.stop) {
            this->mOutput->readfrommixer = Output::mixCallback;
            this->mOutput->mDescription.stop(this->mOutput);
        } else {
            if (this->mOutput->mDescription.polling) {
                dynamic_cast<OutputPolled*>(this->mOutput)->stop();
            }
        }

        if (this->mDSPChannelGroupTarget) {
            this->mDSPChannelGroupTarget->release(true);
            this->mDSPChannelGroupTarget = nullptr;
        }

        if (this->mDSPSoundCard) {
            this->mDSPSoundCard->release(true);
            this->mDSPSoundCard = nullptr;
        }

        auto reverb3d = reinterpret_cast<ReverbI*>(this->mReverb3DHead.getNext());
        while (reverb3d != &this->mReverb3DHead) {
            reverb3d->release(true);
            reverb3d = reinterpret_cast<ReverbI*>(reverb3d->getNext());
        }

        this->mReverb3D.release(false);
        this->mReverbGlobal.release(false);

        for (int32_t i = 0; i < 2; i++) {
            if (this->mDSPTempBuffMem[i]) {
                FMOD_Memory_Free(this->mDSPTempBuffMem[i]);
                this->mDSPTempBuffMem[i] = nullptr;
                this->mDSPTempBuff[i] = nullptr;
            }
        }

        if (this->mOutput->mDescription.close) {
            this->mOutput->readfrommixer = Output::mixCallback;
            this->mOutput->mDescription.close(this->mOutput);
        }

        this->mOutput->release();
        this->mOutput = nullptr;
    }

    if (this->mSoftware) {
        this->mSoftware->release();
        this->mSoftware = nullptr;
    }

    if (this->mEmulated) {
        this->mEmulated->release();
        this->mEmulated = nullptr;
    }

    result = this->mDSPCodecPool_MPEG.close();
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mDSPCodecPool_ADPCM.close();
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mChannel) {
        for (int32_t i = 0; i < this->mNumChannels; i++) {
            if (this->mChannel[i].mLevels) {
                FMOD_Memory_Free(this->mChannel[i].mLevels);
                this->mChannel[i].mLevels = nullptr;
            }
        }
        FMOD_Memory_Free(this->mChannel);
        this->mChannel = nullptr;
    }

    this->mChannelFreeListHead.removeNode();

    result = this->mDSPConnectionPool.close();
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < FMOD_DSP_MAXTREEDEPTH; i++) {
        if (this->mDSPMixBuff[i]) {
            FMOD_Memory_Free(this->mDSPMixBuff[i]);
            this->mDSPMixBuff[i] = nullptr;
        }
    }

    if (this->mDSPCrit) {
        result = FMOD_OS_CriticalSection_Free(this->mDSPCrit);
        if (result != FMOD_OK) {
            return result;
        }

        this->mDSPCrit = nullptr;
    }

    if (this->mDSPLockCrit) {
        result = FMOD_OS_CriticalSection_Free(this->mDSPLockCrit);
        if (result != FMOD_OK) {
            return result;
        }
        this->mDSPLockCrit = nullptr;
    }

    if (this->mDSPConnectionCrit) {
        result = FMOD_OS_CriticalSection_Free(this->mDSPConnectionCrit);
        if (result != FMOD_OK) {
            return result;
        }
        this->mDSPConnectionCrit = nullptr;
    }

    if (this->mDSPRequestCrit) {
        result = FMOD_OS_CriticalSection_Free(this->mDSPRequestCrit);
        if (result != FMOD_OK) {
            return result;
        }
        this->mDSPRequestCrit = nullptr;
    }

    // if (this->mFlags & FMOD_INIT_ENABLE_DSPNET) {
    //     this->mDSPNet.release();
    // }

    if (this->mMultiSubSampleLockBufferCrit) {
        FMOD_OS_CriticalSection_Free(this->mMultiSubSampleLockBufferCrit);
        this->mMultiSubSampleLockBufferCrit = nullptr;
    }

    if (this->mReadDataCrit) {
        FMOD_OS_CriticalSection_Free(this->mReadDataCrit);
        this->mReadDataCrit = nullptr;
    }

    this->mSpeakerLevelsPool.release();

    if (!calledfrominit && this->mInitialized) {
        gGlobal->gSystemInitCount--;
    }

    this->mInitialized = false;
    return FMOD_OK;
}

FMOD_RESULT SystemI::createChannelGroup(const char* name, ChannelGroupI** channelgroup) {
    FMOD_RESULT result;
    ChannelGroupI* newchannelgroup;
    FMOD_DSP_DESCRIPTION_EX description;

    if (!channelgroup) {
        return FMOD_ERR_INVALID_PARAM;
    }

    newchannelgroup = FMOD_Object_Calloc(ChannelGroupI);
    if (!newchannelgroup) {
        return FMOD_ERR_MEMORY;
    }

    newchannelgroup->addAfter(&this->mChannelGroupHead);
    newchannelgroup->mSystem = this;

    if (name) {
        newchannelgroup->mName = FMOD_strdup(name);
        if (!newchannelgroup->mName) {
            return FMOD_ERR_MEMORY;
        }
    } else {
        newchannelgroup->mName = nullptr;
    }

    if (this->mSoftware) {
        FMOD_strcpy(description.name, "ChannelGroup");
        if (name) {
            FMOD_strcat(description.name, ":");
            FMOD_strncat(description.name, name, 18);
        }

        description.channels = 0;
        description.create = nullptr;
        description.release = nullptr;
        description.read = nullptr;
        description.setposition = nullptr;
        description.version = 0x10100;

        result = this->createDSP(&description, &newchannelgroup->mDSPHead);
        if (result != FMOD_OK) {
            return result;
        }

        newchannelgroup->mDSPHead->setDefaults(this->mOutputRate, -1.0f, -1.0f, -1);
        newchannelgroup->mDSPHead->setActive(true);

        result = this->mDSPChannelGroupTarget->addInputQueued(newchannelgroup->mDSPHead, false, nullptr, nullptr);
        if (result != FMOD_OK) {
            return result;
        }
        newchannelgroup->mDSPMixTarget = newchannelgroup->mDSPHead;
    }

    if (name && FMOD_stricmp("music", name) == 0) {
        this->mOutput->mMusicChannelGroup = newchannelgroup;
    }

    *channelgroup = newchannelgroup;

    return FMOD_OK;
}

FMOD_RESULT SystemI::createDSP(FMOD_DSP_DESCRIPTION* description, DSPI** dsp) {
    FMOD_RESULT result;

    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *dsp = nullptr;

    if (!description) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mFlags & FMOD_INIT_DISABLESOFTWARE) {
        return FMOD_ERR_NEEDSSOFTWARE;
    }

    {
        FMOD_DSP_DESCRIPTION_EX descriptionex;

        FMOD_strcpy(descriptionex.name, description->name);

        descriptionex.version = description->version;
        descriptionex.channels = description->channels;
        descriptionex.create = description->create;
        descriptionex.release = description->release;
        descriptionex.reset = description->reset;
        descriptionex.read = description->read;
        descriptionex.setposition = description->setposition;
        descriptionex.numparameters = description->numparameters;
        descriptionex.paramdesc = description->paramdesc;
        descriptionex.setparameter = description->setparameter;
        descriptionex.getparameter = description->getparameter;
        descriptionex.config = description->config;
        descriptionex.configwidth = description->configwidth;
        descriptionex.configheight = description->configheight;
        descriptionex.userdata = description->userdata;

        descriptionex.mSize = 0;
        descriptionex.mCategory = FMOD_DSP_CATEGORY_FILTER;
        descriptionex.mFormat = FMOD_SOUND_FORMAT_PCMFLOAT;
        descriptionex.mModule = nullptr;
        descriptionex.mAEffect = nullptr;
        descriptionex.mResamplerBlockLength = 0;
        descriptionex.mType = FMOD_DSP_TYPE_UNKNOWN;

        result = this->mPluginFactory->createDSP(&descriptionex, dsp);
        if (result != FMOD_OK) {
            return result;
        }
    }

    (*dsp)->mSystem = this;

    return FMOD_OK;
}

FMOD_RESULT SystemI::createDSP(FMOD_DSP_DESCRIPTION_EX* description, DSPI** dsp, bool allocate) {
    FMOD_RESULT result;

    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (allocate) {
        *dsp = nullptr;
    }

    if (!description) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mFlags & FMOD_INIT_DISABLESOFTWARE) {
        return FMOD_ERR_NEEDSSOFTWARE;
    }

    {
        FMOD_DSP_DESCRIPTION_EX descriptionex;

        FMOD_strcpy(descriptionex.name, description->name);

        descriptionex.version = description->version;
        descriptionex.channels = description->channels;
        descriptionex.create = description->create;
        descriptionex.release = description->release;
        descriptionex.reset = description->reset;
        descriptionex.setposition = description->setposition;
        descriptionex.read = description->read;
        descriptionex.numparameters = description->numparameters;
        descriptionex.setparameter = description->setparameter;
        descriptionex.paramdesc = description->paramdesc;
        descriptionex.getparameter = description->getparameter;
        descriptionex.configwidth = description->configwidth;
        descriptionex.config = description->config;
        descriptionex.configheight = description->configheight;
        descriptionex.userdata = description->userdata;

        descriptionex.mSize = description->mSize;
        descriptionex.mCategory = description->mCategory;
        descriptionex.mFormat = description->mFormat;
        descriptionex.mAEffect = description->mAEffect;
        descriptionex.mResamplerBlockLength = description->mResamplerBlockLength;
        descriptionex.mType = description->mType;
        descriptionex.mModule = nullptr;

        result = this->mPluginFactory->createDSP(&descriptionex, dsp);
        if (result != FMOD_OK) {
            return result;
        }
    }

    (*dsp)->mSystem = this;

    return FMOD_OK;
}

FMOD_RESULT SystemI::createDSPByType(FMOD_DSP_TYPE type, DSPI** dsp) {
    FMOD_RESULT result;
    int32_t numdsps;
    FMOD_DSP_DESCRIPTION_EX* descriptionex;

    if (!this->mPluginFactory) {
        return FMOD_ERR_UNINITIALIZED;
    }

    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *dsp = nullptr;

    if (type == FMOD_DSP_TYPE_MIXER) {
        FMOD_DSP_DESCRIPTION desc;
        FMOD_memset(&desc, 0, sizeof(FMOD_DSP_DESCRIPTION));
        FMOD_strcpy(desc.name, "FMOD Mixer unit");

        return this->createDSP(&desc, dsp);
    }

    result = this->mPluginFactory->getNumDSPs(&numdsps);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < numdsps; i++) {
        descriptionex = nullptr;

        result = this->mPluginFactory->getDSP(i, &descriptionex);
        if (result != FMOD_OK) {
            continue;
        }

        if (descriptionex->mType == type) {
            result = this->mPluginFactory->createDSP(descriptionex, dsp);
            if (result != FMOD_OK) {
                return result;
            }

            return FMOD_OK;
        }
    }

    return FMOD_ERR_PLUGIN_MISSING;
}

FMOD_RESULT SystemI::createSample(FMOD_MODE mode, FMOD_CODEC_WAVEFORMAT* waveformat, Sample** sample_out) {
    FMOD_RESULT result;
    Output* output;
    Sample* sample;
    int32_t num2d;
    int32_t num3d;
    int32_t channels;
    int32_t subsamples;
    int32_t samplemaxchannels;
    int32_t loopstart;
    int32_t loopend;
    FMOD_CODEC_WAVEFORMAT subsamplewaveformat;

    if (!sample_out) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (waveformat->lengthpcm == 0xFFFFFFFF) {
        return FMOD_ERR_MEMORY;
    }

    if (!(mode & (FMOD_HARDWARE | FMOD_SOFTWARE))) {
        mode |= (waveformat->mode & (FMOD_HARDWARE | FMOD_SOFTWARE | FMOD_2D | FMOD_3D));
        this->getHardwareChannels(&num2d, &num3d, nullptr);
        if (mode & FMOD_HARDWARE) {
            if (mode & FMOD_3D) {
                if (!num3d) {
                    mode &= ~FMOD_HARDWARE;
                    mode |= FMOD_SOFTWARE;
                }
            } else {
                if (!num2d) {
                    mode &= ~FMOD_HARDWARE;
                    mode |= FMOD_SOFTWARE;
                }
            }
        }
    }

    if (mode & FMOD_SOFTWARE) {
        if (!this->mSoftware) {
            return FMOD_ERR_NEEDSSOFTWARE;
        }

        mode &= ~FMOD_HARDWARE;
        output = this->mSoftware;
    } else {
        mode |= FMOD_HARDWARE;
        output = this->mOutput;
    }

    if (!(mode & (FMOD_2D | FMOD_3D))) {
        mode |= (waveformat->mode & (FMOD_2D | FMOD_3D));
    }

    if (mode & FMOD_3D) {
        mode &= ~FMOD_2D;
    } else {
        mode |= FMOD_2D;
    }

    if (!(mode & (FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI))) {
        mode |= (waveformat->mode & (FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI));
    }

    if (mode & FMOD_LOOP_NORMAL) {
        mode &= ~FMOD_LOOP_OFF;
        mode &= ~FMOD_LOOP_BIDI;
        mode |= FMOD_LOOP_NORMAL;
    } else if (mode & FMOD_LOOP_BIDI) {
        mode &= ~FMOD_LOOP_OFF;
        mode |= FMOD_LOOP_BIDI;
    } else {
        mode |= FMOD_LOOP_OFF;
    }

    if (waveformat->mode & FMOD_CREATECOMPRESSEDSAMPLE) {
        mode |= FMOD_CREATECOMPRESSEDSAMPLE;
        mode &= ~FMOD_CREATESAMPLE;
    }

    channels = waveformat->channels;

    if (channels > FMOD_CHANNEL_MAXREALSUBCHANNELS) {
        if (output->mDescription.getsamplemaxchannels) {
            samplemaxchannels = output->mDescription.getsamplemaxchannels(output, mode, waveformat->format);
        } else {
            samplemaxchannels = 1;
        }

        if (channels > samplemaxchannels) {
            return FMOD_ERR_TOOMANYCHANNELS;
        }
    }

    if (channels > 1) {
        if (output->mDescription.getsamplemaxchannels) {
            samplemaxchannels = output->mDescription.getsamplemaxchannels(output, mode, waveformat->format);
        } else {
            samplemaxchannels = 1;
        }

        if (channels > samplemaxchannels) {
            subsamples = channels;
            if (*sample_out) {
                sample = *sample_out;
            } else {
                sample = FMOD_Object_Calloc(Sample);
            }

            if (!sample) {
                return FMOD_ERR_MEMORY;
            }

            sample->mNumSubSamples = channels;

            if (!sample->mName && !(mode & FMOD_LOWMEM)) {
                sample->mName = reinterpret_cast<char*>(FMOD_Memory_Calloc(FMOD_STRING_MAXNAMELEN));
                if (!sample->mName) {
                    return FMOD_ERR_MEMORY;
                }
            }

            if (sample->mName) {
                FMOD_strcpy(sample->mName, waveformat->name);
            }

            sample->mDefaultFrequency = static_cast<float>(waveformat->frequency);
            sample->mDefaultChannelMask = waveformat->channelmask;
            sample->mLength = waveformat->lengthpcm;
            sample->mMode = mode;
            sample->mLoopStart = 0;
            sample->mLoopLength = sample->mLength;
            sample->mFormat = waveformat->format;
            sample->mCodec = nullptr;
            sample->mChannels = channels;
            sample->mType = FMOD_SOUND_TYPE_USER;
            sample->mSystem = this;
            sample->mMinDistance = this->mDistanceScale;
            sample->mMaxDistance = this->mDistanceScale * 10000.0f;
            sample->mLockBuffer = this->mMultiSubSampleLockBuffer.alloc(SOUND_READCHUNKSIZE);
            if (!sample->mLockBuffer) {
                return FMOD_ERR_MEMORY;
            }
            channels = 1;
        } else {
            subsamples = 1;
        }
    } else {
        subsamples = 1;
    }

    for (int32_t i = 0; i < subsamples; i++) {
        Sample* subsample = subsamples == 1 ? *sample_out : nullptr;
        FMOD_memcpy(&subsamplewaveformat, waveformat, sizeof(FMOD_CODEC_WAVEFORMAT));
        subsamplewaveformat.channels = channels;

        if (output->mDescription.createsample) {
            output->readfrommixer = Output::mixCallback;

            result = output->mDescription.createsample(output, mode, &subsamplewaveformat, &subsample);
        } else {
            result = this->mSoftware->createSample(mode, &subsamplewaveformat, &subsample);
        }

        if (result != FMOD_OK) {
            return result;
        }

        if (subsamples < 2) {
            sample = subsample;

            if (!subsample->mName && !(mode & FMOD_LOWMEM)) {
                subsample->mName = reinterpret_cast<char*>(FMOD_Memory_Calloc(FMOD_STRING_MAXPATHLEN));
                if (!subsample->mName) {
                    return FMOD_ERR_MEMORY;
                }
            }
        } else {
            sample->mSubSample[i] = subsample;
        }

        if (subsample->mName) {
            FMOD_strcpy(subsample->mName, waveformat->name);
        }

        subsample->mDefaultFrequency = static_cast<float>(waveformat->frequency);
        subsample->mDefaultChannelMask = waveformat->channelmask;
        subsample->mMode = mode;
        subsample->mLoopStart = 0;
        subsample->mLoopLength = subsample->mLength;
        subsample->mFormat = waveformat->format;
        subsample->mChannels = channels;
        subsample->mCodec = nullptr;
        subsample->mType = FMOD_SOUND_TYPE_USER;
        subsample->mSystem = this;
        subsample->mMinDistance = this->mDistanceScale;
        subsample->mMaxDistance = this->mDistanceScale * 10000.0f;
        subsample->mSubSampleParent = sample;
    }

    loopstart = waveformat->loopstart;
    loopend = waveformat->loopend;
    if (!loopend) {
        loopend = sample->mLength - 1;
    }

    sample->setLoopPoints(loopstart, FMOD_TIMEUNIT_PCM, loopend, FMOD_TIMEUNIT_PCM);

    *sample_out = sample;

    return FMOD_OK;
}

FMOD_RESULT SystemI::createSound(const char* name_or_data, FMOD_MODE mode_in, FMOD_CREATESOUNDEXINFO* exinfo, SoundI** sound) {
    FMOD_RESULT result;
    SoundI* soundi;

    if (!this->mInitialized) {
        return FMOD_ERR_UNINITIALIZED;
    }
    if (!sound) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (!name_or_data && !(mode_in & FMOD_OPENUSER)) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (!(mode_in & FMOD_SOFTWARE) && !this->mOutput) {
        return FMOD_ERR_OUTPUT_NOHARDWARE;
    }
    if (mode_in & FMOD_HARDWARE && mode_in & FMOD_OPENMEMORY_POINT && !(mode_in & FMOD_CREATESTREAM)) {
        return FMOD_ERR_NEEDSSOFTWARE;
    }

    *sound = nullptr;

    if (!(mode_in & FMOD_NONBLOCKING)) {
        result = this->createSoundInternal(name_or_data, mode_in, exinfo, false, sound);
        if (*sound && exinfo && exinfo->initialsoundgroup) {
            (*sound)->setSoundGroup(reinterpret_cast<SoundGroupI*>(exinfo->initialsoundgroup));
        }
        return result;
    }

    LocalMemoryCounter counter;

    if (mode_in & FMOD_CREATESTREAM) {
        auto stream = FMOD_Object_Calloc(Stream);
        if (!stream) {
            return FMOD_ERR_MEMORY;
        }

        *sound = stream;
    } else {
        Sample* sample = nullptr;
        int32_t hwchannels = 0;
        if (!(mode_in & FMOD_SOFTWARE) && this->getHardwareChannels(nullptr, nullptr, &hwchannels), hwchannels && this->mOutput->mDescription.createsample) {
            this->mOutput->readfrommixer = Output::mixCallback;
            result = this->mOutput->mDescription.createsample(this->mOutput, 0, nullptr, &sample);
        } else {
            result = this->mSoftware->createSample(0, nullptr, &sample);
        }
        if (result != FMOD_OK) {
            return result;
        }
        *sound = sample;
    }

    soundi = reinterpret_cast<SoundI*>(*sound);

    soundi->mAsyncData = reinterpret_cast<AsyncData*>(FMOD_Memory_Calloc(sizeof(AsyncData)));
    if (!soundi->mAsyncData) {
        return FMOD_ERR_MEMORY;
    }

    if (mode_in & (FMOD_OPENMEMORY | FMOD_OPENMEMORY_POINT)) {
        soundi->mAsyncData->mNameData = const_cast<char*>(name_or_data);
    } else if (name_or_data) {
        if (mode_in & FMOD_UNICODE) {
            FMOD_strncpyW(reinterpret_cast<uint16_t*>(soundi->mAsyncData->mName), reinterpret_cast<const uint16_t*>(name_or_data), FMOD_STRING_MAXNAMELEN);
        } else {
            FMOD_strncpy(soundi->mAsyncData->mName, name_or_data, FMOD_STRING_MAXNAMELEN);
        }
    }

    soundi->mMode = mode_in;
    soundi->mSystem = this;
    soundi->mOpenState = FMOD_OPENSTATE_LOADING;
    soundi->mMemoryUsed = counter.getMemoryUsed();

    if (exinfo) {
        FMOD_memcpy(&soundi->mAsyncData->mExInfo, exinfo, sizeof(FMOD_CREATESOUNDEXINFO));
        soundi->mAsyncData->mExInfoExists = true;
        if (exinfo->initialsoundgroup) {
            soundi->setSoundGroup(reinterpret_cast<SoundGroupI*>(exinfo->initialsoundgroup));
        }
    } else {
        soundi->mAsyncData->mExInfoExists = false;
    }

    result = AsyncThread::getAsyncThread(soundi);
    if (result != FMOD_OK) {
        soundi->mOpenState = FMOD_OPENSTATE_ERROR;
        soundi->release(true);
        *sound = nullptr;
        return result;
    }

    FMOD_OS_CriticalSection_Enter(soundi->mAsyncData->mThread->mCrit);
    soundi->mAsyncData->mNode.setData(soundi);
    soundi->mAsyncData->mNode.addBefore(&soundi->mAsyncData->mThread->mHead);
    FMOD_OS_CriticalSection_Leave(soundi->mAsyncData->mThread->mCrit);

    soundi->mAsyncData->mThread->mThread.wakeupThread();

    return FMOD_OK;
}

FMOD_RESULT SystemI::createSoundGroup(const char* name, SoundGroupI** soundgroup) {
    SoundGroupI* newsoundgroup;

    if (!soundgroup) {
        return FMOD_ERR_INVALID_PARAM;
    }

    newsoundgroup = FMOD_Object_Calloc(SoundGroupI);
    if (!newsoundgroup) {
        return FMOD_ERR_MEMORY;
    }

    FMOD_OS_CriticalSection_Enter(gSoundListCrit);
    newsoundgroup->addAfter(&this->mSoundGroupFreeHead);
    newsoundgroup->mSystem = this;
    FMOD_OS_CriticalSection_Leave(gSoundListCrit);

    if (name) {
        newsoundgroup->mName = FMOD_strdup(name);
        if (!newsoundgroup->mName) {
            return FMOD_ERR_MEMORY;
        }
    } else {
        newsoundgroup->mName = nullptr;
    }

    *soundgroup = newsoundgroup;

    return FMOD_OK;
}

FMOD_RESULT SystemI::createSoundInternal(const char* name_or_data, FMOD_MODE mode_in, FMOD_CREATESOUNDEXINFO* exinfo, bool calledfromasync, SoundI** sound) {
    FMOD_RESULT result;
    LocalMemoryCounter counter;
    int32_t num2d;
    int32_t num3d;
    File* file;
    bool netfile = false;
    int32_t numcodecs;
    FMOD_CODEC_DESCRIPTION_EX* codecdesc;
    Codec* codec;
    FMOD_MODE originalmode = mode_in;
    FMOD_CODEC_WAVEFORMAT waveformat;
    FMOD_CODEC_WAVEFORMAT waveformat2;
    FMOD_CODEC_WAVEFORMAT waveformatstream;
    int32_t maxchannels = 0;
    uint32_t filesize;
    Metadata* metadata = nullptr;
    int32_t numsubsounds;
    SoundI** subsound = nullptr;
    SoundI* soundcontainer = nullptr;
    Stream* stream;
    Stream* substream;
    uint32_t blocksize = 0;
    uint32_t decodebuffersize;

    if (!sound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!(mode_in & FMOD_NONBLOCKING)) {
        *sound = 0;
    }

    if (exinfo && exinfo->cbsize != sizeof(FMOD_CREATESOUNDEXINFO)) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (mode_in & 0x80) {
        mode_in &= ~FMOD_CREATESAMPLE;
        mode_in &= ~FMOD_CREATECOMPRESSEDSAMPLE;
    } else if (mode_in & FMOD_CREATECOMPRESSEDSAMPLE) {
        mode_in &= ~FMOD_CREATESAMPLE;
    }

    if (mode_in & FMOD_2D) {
        mode_in &= ~(FMOD_3D_HEADRELATIVE | FMOD_3D_WORLDRELATIVE | FMOD_3D_LOGROLLOFF | FMOD_3D_LINEARROLLOFF | FMOD_3D_CUSTOMROLLOFF);
    }

    if (mode_in & (FMOD_3D_HEADRELATIVE | FMOD_3D_WORLDRELATIVE | FMOD_3D_LOGROLLOFF | FMOD_3D_LINEARROLLOFF | FMOD_3D_CUSTOMROLLOFF)) {
        mode_in |= FMOD_3D;
    }

    if (!(mode_in & FMOD_SOFTWARE)) {
        this->getHardwareChannels(&num2d, &num3d, nullptr);

        if (mode_in & FMOD_3D) {
            if (!num3d) {
                mode_in &= ~FMOD_HARDWARE;
                mode_in |= FMOD_SOFTWARE;
            }
        } else {
            if (!num2d) {
                mode_in &= ~FMOD_HARDWARE;
                mode_in |= FMOD_SOFTWARE;
            }
        }
    }

    if (mode_in & FMOD_OPENUSER || mode_in & FMOD_OPENRAW) {
        if (!exinfo) {
            return FMOD_ERR_INVALID_PARAM;
        }
        if (exinfo->format == FMOD_SOUND_FORMAT_NONE) {
            return FMOD_ERR_INVALID_PARAM;
        }
        if (exinfo->numchannels < 1) {
            return FMOD_ERR_INVALID_PARAM;
        }
        if (exinfo->defaultfrequency == 0) {
            return FMOD_ERR_INVALID_PARAM;
        }
    }

    if (mode_in & (FMOD_OPENMEMORY | FMOD_OPENMEMORY_POINT)) {
        file = FMOD_Object_Alloc(MemoryFile);
        if (!file) {
            return FMOD_ERR_MEMORY;
        }
        file->mBlockSize = 0;
    } else if (mode_in & FMOD_OPENUSER) {
        file = FMOD_Object_Alloc(NullFile);
        if (!file) {
            return FMOD_ERR_MEMORY;
        }
        mode_in &= ~FMOD_OPENRAW;
    } else if (exinfo && exinfo->useropen && exinfo->userclose && exinfo->userread && exinfo->userseek) {
        file = FMOD_Object_Alloc(UserFile);
        if (!file) {
            return FMOD_ERR_MEMORY;
        }

        result = reinterpret_cast<UserFile*>(file)->setUserCallbacks(exinfo->useropen, exinfo->userclose, exinfo->userread, exinfo->userseek);
        if (result != FMOD_OK) {
            return result;
        }
    } else if (File::gUsesUserCallbacks) {
        file = FMOD_Object_Alloc(UserFile);
        if (!file) {
            return FMOD_ERR_MEMORY;
        }

    } else if (!FMOD_strnicmp("http://", name_or_data, 7) || !FMOD_strnicmp("http:\\\\", name_or_data, 7) || !FMOD_strnicmp("https://", name_or_data, 8) || !FMOD_strnicmp("https:\\\\", name_or_data, 8) || !FMOD_strnicmp("mms://", name_or_data, 6) || !FMOD_strnicmp("mms:\\\\", name_or_data, 6)) {
        // file = FMOD_Object_Alloc(NetFile);
        // if (!file) {
        //     return FMOD_ERR_MEMORY;
        // }
        //
        // netfile = true;

        return FMOD_ERR_UNSUPPORTED;
    } else if (FMOD_OS_CDDA_IsDeviceName(name_or_data)) {
        // file = FMOD_Object_Alloc(CddaFile);
        // if (!file) {
        //     return FMOD_ERR_MEMORY;
        // }
        //
        // reinterpret_cast<CddaFile*>(file)->init(mode_in & FMOD_CDDA_FORCEASPI, mode_in & FMOD_CDDA_JITTERCORRECT);

        return FMOD_ERR_UNSUPPORTED;
    } else {
        file = FMOD_Object_Alloc(DiskFile);
        if (!file) {
            return FMOD_ERR_MEMORY;
        }
    }

    file->mSystem = this;

    if (*sound && netfile) {
        (*sound)->mOpenState = FMOD_OPENSTATE_CONNECTING;
    }

    result = file->open(name_or_data, exinfo ? exinfo->length : 0, mode_in & FMOD_UNICODE, exinfo ? exinfo->encryptionkey : nullptr);
    if (result != FMOD_OK) {
        return result;
    }

    result = file->setStartOffset(exinfo ? exinfo->fileoffset : 0);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mPluginFactory->getNumCodecs(&numcodecs);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < numcodecs; i++) {
        result = this->mPluginFactory->getCodec(i, &codecdesc);
        if (result != FMOD_OK) {
            continue;
        }

        if (exinfo) {
            if (exinfo->cbsize > offsetof(FMOD_CREATESOUNDEXINFO, suggestedsoundtype)) {
                if (exinfo->suggestedsoundtype) {
                    if (!(mode_in & FMOD_OPENUSER) && codecdesc->mType != exinfo->suggestedsoundtype) {
                        continue;
                    }
                }
            }
        }
        if (mode_in & FMOD_OPENRAW) {
            if (codecdesc->mType != FMOD_SOUND_TYPE_RAW) {
                codecdesc = nullptr;
                continue;
            }
        } else if (mode_in & FMOD_OPENUSER) {
            if (codecdesc->mType != FMOD_SOUND_TYPE_USER) {
                codecdesc = nullptr;
                continue;
            }
        } else if (codecdesc->mType == FMOD_SOUND_TYPE_RAW || codecdesc->mType == FMOD_SOUND_TYPE_USER) {
            codecdesc = nullptr;
            continue;
        }

        result = this->mPluginFactory->createCodec(codecdesc, &codec);
        if (result != FMOD_OK) {
            if (result == FMOD_ERR_MEMORY) {
                return result;
            }
        }

        codec->mFile = file;
        codec->mMode = mode_in;
        codec->mOriginalMode = originalmode;
        codec->filehandle = file;
        codec->filesize = file->mLength;
        codec->mSystem = this;
        codec->mFlags |= FMOD_CODEC_ACCURATELENGTH;

        result = codec->mDescription.open(codec, mode_in, exinfo);
        if (result == FMOD_OK) {
            mode_in = codec->mMode;

            result = codec->mDescription.getwaveformat(codec, 0, &waveformat);
            if (result != FMOD_OK) {
                return result;
            }

            if (mode_in & FMOD_OPENUSER) {
                waveformat.format = exinfo->format;
                waveformat.channels = exinfo->numchannels;
                waveformat.frequency = exinfo->defaultfrequency;

                result = SoundI::getSamplesFromBytes(exinfo->length, &waveformat.lengthpcm, exinfo->numchannels, exinfo->format);
                if (result != FMOD_OK) {
                    return result;
                }
                codec->waveformat[0].blockalign = 1;
                waveformat.blockalign = 1;
            } else if (mode_in & FMOD_OPENRAW) {
                file->getSize(&filesize);

                codec->waveformat[0].format = exinfo->format;
                waveformat.format = exinfo->format;

                codec->waveformat[0].channels = exinfo->numchannels;
                waveformat.channels = exinfo->numchannels;

                codec->waveformat[0].frequency = exinfo->defaultfrequency;
                waveformat.frequency = exinfo->defaultfrequency;

                result = SoundI::getSamplesFromBytes(filesize, &waveformat.lengthpcm, exinfo->numchannels, exinfo->format);
                if (result != FMOD_OK) {
                    return result;
                }
            }

            maxchannels = waveformat.channels;
            codec->mBlockAlign = waveformat.blockalign;

            if (codec->numsubsounds && mode_in & FMOD_CREATESTREAM) {
                for (int32_t j = 0; j < codec->numsubsounds; j++) {
                    result = codec->mDescription.getwaveformat(codec, j, &waveformat2);
                    if (result != FMOD_OK) {
                        return result;
                    }

                    if (waveformat2.channels > maxchannels) {
                        maxchannels = waveformat2.channels;
                        codec->mBlockAlign = waveformat2.blockalign;
                    }
                }
            }
            break;
        }

        if (codec->mType == FMOD_SOUND_TYPE_TAG) {
            metadata = codec->mMetadata;
        }

        codec->mFile = nullptr;
        codec->mMetadata = nullptr;
        codec->release();
        codec = nullptr;

        if (result != FMOD_ERR_FORMAT && result != FMOD_ERR_FILE_EOF) {
            file->close();
            FMOD_Memory_Free(file);
            if (metadata) {
                metadata->release();
            }
            return result;
        }
    }

    if (!codec) {
        file->close();
        FMOD_Memory_Free(file);
        if (metadata) {
            metadata->release();
        }
        return FMOD_ERR_FORMAT;
    }

    numsubsounds = exinfo && exinfo->numsubsounds ? exinfo->numsubsounds : codec->numsubsounds;
    if (metadata) {
        if (codec->mMetadata) {
            codec->mMetadata->add(metadata);
            metadata->release();
        } else {
            codec->mMetadata = metadata;
        }
    }

    codec->getMetadataFromFile();

    if (numsubsounds > 0) {
        subsound = reinterpret_cast<SoundI**>(FMOD_Memory_Calloc(numsubsounds * sizeof(SoundI*)));
        if (!subsound) {
            return FMOD_ERR_MEMORY;
        }
    }

    result = codec->mDescription.getwaveformat(codec, 0, &waveformat);
    if (result != FMOD_OK) {
        return result;
    }

    if (waveformat.channels > this->mMaxInputChannels) {
        return FMOD_ERR_TOOMANYCHANNELS;
    }

    if (codec->mDescription.defaultasstream && !(mode_in & FMOD_CREATESAMPLE)) {
        mode_in |= FMOD_CREATESTREAM;
    }

    if (codec->mType == FMOD_SOUND_TYPE_PLAYLIST) {
        if (mode_in & FMOD_NONBLOCKING) {
            soundcontainer = *sound;
        } else {
            soundcontainer = FMOD_Object_Calloc(Sample);
            if (!soundcontainer) {
                return FMOD_ERR_MEMORY;
            }
        }

        if (!soundcontainer->mName && !(mode_in & FMOD_LOWMEM)) {
            soundcontainer->mName = reinterpret_cast<char*>(FMOD_Memory_Calloc(FMOD_STRING_MAXNAMELEN));
            if (!soundcontainer->mName) {
                return FMOD_ERR_MEMORY;
            }
        }

        soundcontainer->mSystem = this;
        soundcontainer->mPostReadCallback = exinfo ? exinfo->pcmreadcallback : nullptr;
        soundcontainer->mPostSetPositionCallback = exinfo ? exinfo->pcmsetposcallback : nullptr;
        soundcontainer->mPostCallbackSound = reinterpret_cast<FMOD_SOUND*>(soundcontainer);
        soundcontainer->mType = codec->mType;
        soundcontainer->mCodec = codec;
        soundcontainer->mMinDistance = this->mDistanceScale;
        soundcontainer->mMaxDistance = this->mDistanceScale * 10000.0f;
        soundcontainer->mSubSound = nullptr;
        soundcontainer->mNumSubSounds = 0;
        soundcontainer->mSubSoundIndex = -1;
        soundcontainer->mUserData = exinfo ? exinfo->userdata : nullptr;

        *sound = soundcontainer;
    } else if (mode_in & FMOD_CREATESTREAM) {
        FMOD_MODE sample_mode = mode_in & ~FMOD_CREATECOMPRESSEDSAMPLE;
        Sample* sample = nullptr;

        if (mode_in & FMOD_NONBLOCKING) {
            stream = reinterpret_cast<Stream*>(sound);
        } else {
            stream = FMOD_Object_Alloc(Stream);
            if (!stream) {
                if (subsound) {
                    FMOD_Memory_Free(subsound);
                }
                codec->release();
                return FMOD_ERR_MEMORY;
            }
        }

        if (!stream->mName && !(mode_in & FMOD_LOWMEM)) {
            stream->mName = reinterpret_cast<char*>(FMOD_Memory_Calloc(FMOD_STRING_MAXNAMELEN));
            if (!stream->mName) {
                if (subsound) {
                    FMOD_Memory_Free(subsound);
                }
                codec->release();
                return FMOD_ERR_MEMORY;
            }
        }

        stream->mSubSound = subsound;
        stream->mNumSubSounds = numsubsounds;
        stream->mCodec = codec;
        stream->mSystem = this;

        stream->getSamplesFromBytes(codec->mBlockAlign, &blocksize, maxchannels, waveformat.format);

        decodebuffersize = exinfo ? exinfo->decodebuffersize : 0;
        if (!decodebuffersize) {
            decodebuffersize = (400 * waveformat.frequency) / 1000;
        }
        if (!blocksize) {
            blocksize = decodebuffersize;
        }
        if (blocksize < 256) {
            if (!blocksize) {
                blocksize = 1;
            }
            // XXX: review this
            blocksize *= (blocksize + 255) / blocksize;
        }

        decodebuffersize /= blocksize;
        decodebuffersize *= blocksize;
        if (decodebuffersize <= blocksize) {
            decodebuffersize = blocksize * 2;
        }

        // ...

        sample_mode &= ~FMOD_LOOP_OFF;
        sample_mode &= ~FMOD_LOOP_BIDI;
        sample_mode &= ~FMOD_OPENMEMORY_POINT;
        sample_mode |= FMOD_LOOP_NORMAL;

        FMOD_memcpy(&waveformatstream, &waveformat, sizeof(FMOD_CODEC_WAVEFORMAT));

        if (exinfo && exinfo->speakermap) {
            if (exinfo->speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
                waveformatstream.channelmask = SPEAKER_ALLMONO;
            } else if (exinfo->speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO) {
                waveformatstream.channelmask = SPEAKER_ALLSTEREO;
            }
        }

        waveformatstream.lengthpcm = decodebuffersize;
        waveformatstream.loopend = decodebuffersize - 1;
        waveformatstream.channels = maxchannels;
        waveformatstream.loopstart = 0;

        result = this->createSample(sample_mode, &waveformatstream, &sample);
        if (result != FMOD_OK) {
            return result;
        }

        sample->mPostReadCallback = exinfo ? exinfo->pcmreadcallback : nullptr;
        sample->mPostSetPositionCallback = exinfo ? exinfo->pcmsetposcallback : nullptr;
        sample->mUserData = exinfo ? exinfo->userdata : nullptr;
        sample->mPostCallbackSound = reinterpret_cast<FMOD_SOUND*>(stream);
        sample->mCodec = codec;

        for (int32_t i = 0; i < sample->mNumSubSamples; i++) {
            sample->mSubSample[i]->mCodec = codec;
        }

        stream->mSample = sample;
        stream->mSubSoundIndex = 0;
        stream->mLength = waveformat.lengthpcm;
        stream->mLengthBytes = waveformat.lengthbytes;
        stream->mDefaultFrequency = static_cast<float>(waveformat.frequency);
        stream->mMode = (sample_mode | FMOD_UNIQUE | FMOD_CREATESTREAM) & ~FMOD_LOOP_NORMAL;
        stream->mFormat = waveformat.format;
        stream->mChannels = maxchannels;
        stream->mSystem = this;
        stream->mPostReadCallback = exinfo ? exinfo->pcmreadcallback : nullptr;
        stream->mPostSetPositionCallback = exinfo ? exinfo->pcmsetposcallback : nullptr;
        stream->mPostCallbackSound = reinterpret_cast<FMOD_SOUND*>(stream);
        stream->mBlockSize = blocksize;
        stream->mType = codec->mType;
        stream->mMinDistance = this->mDistanceScale;
        stream->mMaxDistance = this->mDistanceScale * 10000.0f;
        stream->mUserData = exinfo ? exinfo->userdata : nullptr;
        if (stream->mName) {
            FMOD_strcpy(stream->mName, waveformat.name);
        }

        if (exinfo && exinfo->speakermap) {
            if (exinfo->speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
                sample->mDefaultChannelMask = SPEAKER_ALLMONO;
            } else if (exinfo->speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO) {
                sample->mDefaultChannelMask = SPEAKER_ALLSTEREO;
            }
        } else {
            sample->mDefaultChannelMask = waveformat.channelmask;
        }

        stream->mMode &= ~(FMOD_2D | FMOD_3D | FMOD_HARDWARE | FMOD_SOFTWARE);
        stream->mMode |= (sample->mMode & (FMOD_2D | FMOD_3D | FMOD_HARDWARE | FMOD_SOFTWARE));

        if (waveformat.mode & FMOD_LOOP_OFF || mode_in & FMOD_LOOP_OFF) {
            stream->mMode |= FMOD_LOOP_OFF;
        } else if (waveformat.mode & FMOD_LOOP_NORMAL || mode_in & FMOD_LOOP_NORMAL) {
            stream->mMode |= FMOD_LOOP_NORMAL;
        } else {
            stream->mMode |= FMOD_LOOP_OFF;
        }
        stream->setLoopPoints(waveformat.loopstart, FMOD_TIMEUNIT_PCM, waveformat.loopend, FMOD_TIMEUNIT_PCM);

        if (!stream->mSoundGroupNode.getData()) {
            result = stream->setSoundGroup(this->mSoundGroup);
            if (result != FMOD_OK) {
                return result;
            }
        }

        stream->mChannel = FMOD_Object_Calloc(ChannelStream);
        if (!stream->mChannel) {
            if (!(stream->mMode & FMOD_NONBLOCKING)) {
                stream->release(true);
            }
            return FMOD_ERR_MEMORY;
        }

        FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);
        stream->mStreamNode.setData(stream);
        stream->mStreamNode.addBefore(&this->mStreamListSoundHead);
        FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);

        if (numsubsounds > 0 && !(mode_in & FMOD_OPENUSER)) {
            substream = FMOD_Object_Calloc(Stream);
            if (!substream) {
                if (!(stream->mMode & FMOD_NONBLOCKING)) {
                    stream->release(true);
                }
                return FMOD_ERR_MEMORY;
            }

            if (numsubsounds > 1) {
                stream->mSubSoundShared = substream;
            }

            if (!(mode_in & FMOD_LOWMEM)) {
                substream->mName = reinterpret_cast<char*>(FMOD_Memory_Calloc(FMOD_STRING_MAXNAMELEN));
                if (!substream->mName) {
                    if (!(stream->mMode & FMOD_NONBLOCKING)) {
                        stream->release(true);
                    }
                    return FMOD_ERR_MEMORY;
                }
            }

            substream->mSample = sample;
            substream->mCodec = codec;
            substream->mSubSoundIndex = 0;
            substream->mChannel = stream->mChannel;
            substream->mMode = stream->mMode;
            substream->mType = stream->mType;
            substream->mAsyncData = stream->mAsyncData;
            substream->mMinDistance = this->mDistanceScale;
            substream->mMaxDistance = this->mDistanceScale * 10000.0f;
            substream->mBlockSize = blocksize;
            substream->mSubSoundParent = stream;
            substream->mSubSoundShared = substream;

            if (!substream->mSoundGroupNode.getData()) {
                result = substream->setSoundGroup(this->mSoundGroup);
                if (result != FMOD_OK) {
                    return result;
                }
            }

            for (int32_t i = 0; i < numsubsounds; i++) {
                subsound[i] = substream;

                if (i < codec->numsubsounds) {
                    result = codec->mDescription.getwaveformat(codec, i, &waveformat2);
                    if (result != FMOD_OK) {
                        return result;
                    }

                    if (substream->mName) {
                        FMOD_strcpy(substream->mName, waveformat2.name);
                    }
                    substream->mDefaultChannelMask = waveformat2.channelmask;
                    substream->mDefaultFrequency = static_cast<float>(waveformat2.frequency);
                    substream->mChannels = waveformat2.channels;
                    substream->mFormat = waveformat2.format;
                    substream->mLoopLength = waveformat2.loopend - waveformat2.loopstart + 1;
                    substream->mLength = waveformat2.lengthpcm;
                    substream->mLengthBytes = waveformat2.lengthbytes;
                    substream->mLoopStart = waveformat2.loopstart;
                    substream->setLoopPoints(waveformat2.loopstart, FMOD_TIMEUNIT_PCM, waveformat2.loopend, FMOD_TIMEUNIT_PCM);

                    if (codec->mDescription.soundcreate) {
                        result = codec->mDescription.soundcreate(codec, i, reinterpret_cast<FMOD_SOUND*>(substream));
                        if (result != FMOD_OK) {
                            return result;
                        }
                    }
                }

                stream->mNumActiveSubSounds++;
            }

            if (codec->numsubsounds) {
                substream->updateSubSound(0, false);
            }

            if (!(mode_in & FMOD_LOWMEM) && !(exinfo && exinfo->initialsubsound)) {
                stream->setSubSoundSentence(nullptr, numsubsounds);
            }

        } else {
            if (codec->mDescription.soundcreate) {
                result = codec->mDescription.soundcreate(codec, 0, reinterpret_cast<FMOD_SOUND*>(stream));
                if (result != FMOD_OK) {
                    return result;
                }
            }
        }

        if (exinfo) {
            // ... ?
            switch (exinfo->initialseekpostype) {
            case 0:
            case FMOD_TIMEUNIT_PCM: {
                break;
            }
            case FMOD_TIMEUNIT_MS: {
                exinfo->initialseekposition = static_cast<uint32_t>(static_cast<float>(exinfo->initialseekposition) / 1000.0f * stream->mDefaultFrequency);
                exinfo->initialseekpostype = FMOD_TIMEUNIT_PCM;
                break;
            }
            case FMOD_TIMEUNIT_PCMBYTES: {
                stream->getSamplesFromBytes(exinfo->initialseekposition, &exinfo->initialseekposition);
                exinfo->initialseekpostype = FMOD_TIMEUNIT_PCM;
                break;
            }
            default: {
                return FMOD_ERR_INVALID_PARAM;
            }
            }
        }

        if (exinfo && exinfo->initialsubsound && stream->mSubSound) {
            if (exinfo->initialsubsound < 0 || exinfo->initialsubsound >= stream->mNumSubSounds) {
                if (!(stream->mMode & FMOD_NONBLOCKING)) {
                    stream->release(true);
                }
                return FMOD_ERR_INVALID_PARAM;
            }

            stream->mSubSoundIndex = exinfo->initialsubsound;
            stream->mSubSoundListCurrent = exinfo->initialsubsound;

            substream = reinterpret_cast<Stream*>(stream->mSubSound[stream->mSubSoundIndex]);
            if (substream) {
                stream->mSubSoundIndex = exinfo->initialsubsound;
                stream->mSubSoundListCurrent = exinfo->initialsubsound;
                if (substream->mSubSoundShared) {
                    result = substream->updateSubSound(exinfo->initialsubsound, false);
                    if (result != FMOD_OK) {
                        if (!(stream->mMode & FMOD_NONBLOCKING)) {
                            stream->release(true);
                        }
                        return result;
                    }
                }

                stream->mInitialPosition = exinfo->initialseekposition;
                result = substream->setPosition(exinfo->initialseekposition, FMOD_TIMEUNIT_PCM);
                if (result != FMOD_OK) {
                    if (!(stream->mMode & FMOD_NONBLOCKING)) {
                        stream->release(true);
                    }
                    return result;
                }
            }
        } else {
            stream->mInitialPosition = exinfo ? exinfo->initialseekpostype : 0;
            result = stream->setPosition(stream->mInitialPosition, FMOD_TIMEUNIT_PCM);
            if (result != FMOD_OK) {
                return result;
            }
        }

        if (!(mode_in & FMOD_OPENONLY)) {
            result = stream->flush();
            if (result != FMOD_OK) {
                if (!(stream->mMode & FMOD_NONBLOCKING)) {
                    stream->release(true);
                }
                return result;
            }
        }

        auto buffersizetype = this->mStreamFileBufferSizeType;
        auto buffersize = this->mStreamFileBufferSize;

        uint32_t sizebytes = 0;

        if (buffersizetype == FMOD_TIMEUNIT_RAWBYTES) {
            sizebytes = buffersize;
        } else {
            uint32_t pcm = buffersize;

            if (buffersizetype == FMOD_TIMEUNIT_MS) {
                pcm = buffersize * (static_cast<uint32_t>(stream->mDefaultFrequency) / 1000);
            } else if (buffersizetype == FMOD_TIMEUNIT_PCMBYTES) {
                stream->getSamplesFromBytes(buffersize, &pcm);
            }

            if (stream->mLength == static_cast<uint32_t>(-1) || file->mLength == static_cast<uint32_t>(-1)) {
                stream->getBytesFromSamples(pcm, &sizebytes);
            } else {
                float frac;

                if (stream->mLengthBytes) {
                    frac = static_cast<float>(stream->mLengthBytes) / static_cast<float>(stream->mLength);
                } else {
                    frac = static_cast<float>(file->mLength) / static_cast<float>(stream->mLength);
                }

                sizebytes = static_cast<uint32_t>(static_cast<float>(pcm) * frac);
            }
        }

        if (!(mode_in & FMOD_OPENUSER)) {
            result = file->enableDoubleBuffer(sizebytes);
            if (result != FMOD_OK) {
                if (!(stream->mMode & FMOD_NONBLOCKING)) {
                    stream->release(true);
                }
                return result;
            }
        }

        *sound = stream;
    } else {
        Sample* sample;
        Sample* samplecontainer = nullptr;
        bool realtimesample = false;

        if (numsubsounds > 0) {
            if (mode_in & FMOD_NONBLOCKING) {
                samplecontainer = reinterpret_cast<Sample*>(*sound);
            } else {
                samplecontainer = FMOD_Object_Calloc(Sample);
                if (!samplecontainer) {
                    return FMOD_ERR_MEMORY;
                }
            }

            if (!samplecontainer->mName && !(mode_in & FMOD_LOWMEM)) {
                samplecontainer->mName = reinterpret_cast<char*>(FMOD_Memory_Calloc(FMOD_STRING_MAXNAMELEN));
                if (!samplecontainer->mName) {
                    return FMOD_ERR_MEMORY;
                }
            }

            samplecontainer->mSystem = this;
            samplecontainer->mPostReadCallback = exinfo ? exinfo->pcmreadcallback : nullptr;
            samplecontainer->mPostSetPositionCallback = exinfo ? exinfo->pcmsetposcallback : nullptr;
            samplecontainer->mPostCallbackSound = reinterpret_cast<FMOD_SOUND*>(samplecontainer);
            samplecontainer->mType = codec->mType;
            samplecontainer->mCodec = codec;
            samplecontainer->mMinDistance = this->mDistanceScale;
            samplecontainer->mMaxDistance = this->mDistanceScale * 10000.0f;
            samplecontainer->mSubSound = subsound;
            samplecontainer->mNumSubSounds = numsubsounds;
            samplecontainer->mSubSoundIndex = -1;
            samplecontainer->mUserData = exinfo ? exinfo->userdata : nullptr;
        }

        for (int32_t i = 0; (i < (numsubsounds == 0 ? 1 : numsubsounds)) && (i < (numsubsounds == 0 ? 1 : codec->numsubsounds)); i++) {
            auto sample_mode = mode_in;

            result = codec->mDescription.getwaveformat(codec, i, &waveformat2);
            if (result != FMOD_OK) {
                return result;
            }

            if (!numsubsounds && (sample_mode & FMOD_NONBLOCKING)) {
                sample = reinterpret_cast<Sample*>(*sound);
            } else {
                sample = nullptr;
            }

            if (numsubsounds > 0 && exinfo && exinfo->inclusionlist) {
                bool found = false;
                for (int32_t j = 0; j < exinfo->inclusionlistnum; j++) {
                    if (exinfo->inclusionlist[j] == i) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    continue;
                }
            }

            if (exinfo && exinfo->speakermap) {
                if (exinfo->speakermap == FMOD_SPEAKERMAPTYPE_ALLMONO) {
                    waveformat2.channelmask = SPEAKER_ALLMONO;
                } else if (exinfo->speakermap == FMOD_SPEAKERMAPTYPE_ALLSTEREO) {
                    waveformat2.channelmask = SPEAKER_ALLSTEREO;
                }
            }

            if (sample_mode & FMOD_CREATECOMPRESSEDSAMPLE) {
                if (waveformat2.format == FMOD_SOUND_FORMAT_MPEG ||
                    waveformat2.format == FMOD_SOUND_FORMAT_IMAADPCM ||
                    waveformat2.format == FMOD_SOUND_FORMAT_XMA) {
                    realtimesample = true;
                } else {
                    sample_mode &= ~FMOD_CREATECOMPRESSEDSAMPLE;

                    if (samplecontainer) {
                        samplecontainer->mMode &= ~FMOD_CREATECOMPRESSEDSAMPLE;
                    }
                }
            }

            if (!realtimesample && mode_in & FMOD_OPENMEMORY_POINT) {
                result = codec->canPointTo();
                if (result != FMOD_OK) {
                    return result;
                }
            }

            result = this->createSample(sample_mode, &waveformat2, &sample);
            if (result != FMOD_OK) {
                if (samplecontainer) {
                    samplecontainer->release(true);
                }
                if (codec) {
                    codec->release();
                }
                return result;
            }

            sample->mSubSoundIndex = i;
            sample->mSubSoundParent = samplecontainer;
            sample->mType = codec->mType;
            sample->mCodec = codec;
            sample->mMinDistance = this->mDistanceScale;
            sample->mMaxDistance = this->mDistanceScale * 10000.0f;
            sample->mUserData = exinfo ? exinfo->userdata : nullptr;

            if (sample->mNumSubSamples) {
                for (int32_t j = 0; j < sample->mNumSubSamples; j++) {
                    sample->mSubSample[j]->mCodec = codec;
                    sample->mSubSample[j]->mSubSoundIndex = sample->mSubSoundIndex;
                }
            }

            if (!sample->mSoundGroupNode.getData()) {
                result = sample->setSoundGroup(this->mSoundGroup);
                if (result != FMOD_OK) {
                    return result;
                }
            }

            if (!numsubsounds && i == 0) {
                sample->mPostReadCallback = exinfo ? exinfo->pcmreadcallback : nullptr;
                sample->mPostSetPositionCallback = exinfo ? exinfo->pcmsetposcallback : nullptr;
                sample->mPostCallbackSound = reinterpret_cast<FMOD_SOUND*>(sample);
            }

            if (codec->mDescription.soundcreate) {
                result = codec->mDescription.soundcreate(codec, i, reinterpret_cast<FMOD_SOUND*>(sample));
                if (result != FMOD_OK) {
                    return result;
                }
            }

            result = codec->reset();
            if (result != FMOD_OK) {
                return result;
            }

            FMOD_TIMEUNIT settimeunit = (waveformat2.mode & FMOD_CREATECOMPRESSEDSAMPLE || sample_mode & FMOD_CREATECOMPRESSEDSAMPLE) ? FMOD_TIMEUNIT_RAWBYTES : FMOD_TIMEUNIT_PCM;
            result = codec->setPosition(i, 0, settimeunit);
            if (result != FMOD_OK) {
                return result;
            }

            if (sample->mPostSetPositionCallback) {
                if (numsubsounds > 0) {
                    samplecontainer->mPostSetPositionCallback(reinterpret_cast<FMOD_SOUND*>(samplecontainer), i, 0, FMOD_TIMEUNIT_PCM);
                } else {
                    sample->mPostSetPositionCallback(reinterpret_cast<FMOD_SOUND*>(sample), 0, 0, FMOD_TIMEUNIT_PCM);
                }
            }

            if (sample_mode & FMOD_OPENMEMORY_POINT) {
                result = sample->setBufferData(const_cast<char*>(name_or_data) + codec->mFile->mCurrentPosition);
                if (result != FMOD_OK) {
                    if (samplecontainer) {
                        samplecontainer->release(true);
                    }
                    if (samplecontainer || codec) {
                        sample->release(true);
                    }
                    return result;
                }

            } else if (!(sample_mode & FMOD_OPENONLY)) {
                uint32_t samplelength;
                uint32_t read;

                if (waveformat2.mode & FMOD_CREATECOMPRESSEDSAMPLE || sample_mode & FMOD_CREATECOMPRESSEDSAMPLE) {
                    samplelength = waveformat2.lengthbytes;

                    if (codec->mType != FMOD_SOUND_TYPE_FSB && codec->mType != FMOD_SOUND_TYPE_XMA) {
                        samplelength -= codec->mSrcDataOffset;
                    }
                } else {
                    samplelength = waveformat2.lengthpcm;
                }

                result = sample->read(0, samplelength, &read);
                if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
                    if (!(sample->mMode & FMOD_NONBLOCKING)) {
                        sample->release(true);
                        samplecontainer->release(true);
                        return result;
                    }
                    if (codec) {
                        sample->release(true);
                    }
                    return result;
                }

                if (samplelength != read && sample->mLoopLength == sample->mLength) {
                    sample->mLoopLength = read;
                }

                result = sample->setPosition(0);
            }

            if (numsubsounds > 0) {
                subsound[i] = sample;
                samplecontainer->mNumActiveSubSounds++;
            }
        }

        if (!(mode_in & FMOD_OPENONLY)) {
            file->close();
            FMOD_Memory_Free(file);
            codec->mFile = nullptr;
        }

        if (numsubsounds > 0) {
            *sound = samplecontainer;
        } else {
            *sound = sample;
        }
    }

    auto soundi = reinterpret_cast<SoundI*>(*sound);
    FMOD_OS_CriticalSection_Enter(gSoundListCrit);
    soundi->addAfter(&this->mSoundListHead);
    FMOD_OS_CriticalSection_Leave(gSoundListCrit);

    if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
        if (!(mode_in & FMOD_NONBLOCKING)) {
            *sound = nullptr;
        }

        return result;
    }

    if (soundi->mName && ((mode_in & FMOD_UNICODE && !(reinterpret_cast<int16_t*>(soundi->mName)[0])) || !soundi->mName[0])) {
        bool usefilename = true;

        if (codec->mMetadata) {
            FMOD_TAG tag;

            result = codec->mMetadata->getTag("TITLE", 0, &tag);
            if (result == FMOD_OK) {
                FMOD_strncpy(soundi->mName, reinterpret_cast<char*>(tag.data), FMOD_STRING_MAXNAMELEN);
                usefilename = false;
                codec->mMetadata->addTag(tag.type, "TITLE", tag.data, tag.datalen, tag.datatype, true);
            } else {
                result = codec->mMetadata->getTag("TT2", 0, &tag);
                if (result == FMOD_OK) {
                    FMOD_strncpy(soundi->mName, reinterpret_cast<char*>(tag.data), FMOD_STRING_MAXNAMELEN);
                    usefilename = false;
                    codec->mMetadata->addTag(tag.type, "TT2", tag.data, tag.datalen, tag.datatype, true);
                }
            }
        }

        if (usefilename && !(mode_in & (FMOD_OPENMEMORY | FMOD_OPENMEMORY_POINT)) && name_or_data) {
            int32_t count;
            if (mode_in & FMOD_UNICODE) {
                for (count = FMOD_strlenW(reinterpret_cast<const uint16_t*>(name_or_data)); count >= 0; count--) {
                    if ((reinterpret_cast<const uint16_t*>(name_or_data))[count] == L'\\' || (reinterpret_cast<const uint16_t*>(name_or_data))[count] == L'/') {
                        count++;
                        break;
                    }
                }
                if (count < 0) {
                    count = 0;
                }

                FMOD_strncpyW(reinterpret_cast<uint16_t*>(soundi->mName), reinterpret_cast<const uint16_t*>(name_or_data) + count, FMOD_STRING_MAXNAMELEN / 2);
            } else {
                for (count = FMOD_strlen(name_or_data); count >= 0; count--) {
                    if (name_or_data[count] == '\\' || name_or_data[count] == '/') {
                        count++;
                        break;
                    }
                }
                if (count < 0) {
                    count = 0;
                }

                FMOD_strncpy(soundi->mName, reinterpret_cast<const char*>(name_or_data) + count, FMOD_STRING_MAXNAMELEN);
            }
        }
    }

    if (!calledfromasync) {
        soundi->mOpenState = FMOD_OPENSTATE_READY;
    }

    soundi->mMemoryUsed += counter.getMemoryUsed();

    return FMOD_OK;
}

FMOD_RESULT SystemI::createStream(const char* name_or_data, FMOD_MODE mode_in, FMOD_CREATESOUNDEXINFO* exinfo, SoundI** sound) {
    return this->createSound(name_or_data, mode_in | FMOD_CREATESTREAM, exinfo, sound);
}

FMOD_RESULT SystemI::findChannel(FMOD_CHANNELINDEX id, DSPI* dsp, ChannelI** channel) {
    ChannelReal* realchannel[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { 0 };
    ChannelI* chan;
    FMOD_RESULT result;

    if (!channel) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (id == FMOD_CHANNEL_REUSE) {
        chan = *channel;
        if (!chan) {
            id = FMOD_CHANNEL_FREE;
        } else {
            if (chan->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_STOPPED) {
                id = static_cast<FMOD_CHANNELINDEX>(chan->mIndex);
            } else {
                chan->stopEx(false, false, false, true, false, true, false);
            }
        }
    }

    if (id != FMOD_CHANNEL_REUSE) {
        *channel = nullptr;

        if (id != FMOD_CHANNEL_FREE) {
            chan = &this->mChannel[id];
            chan->stop();
        } else {
            if (!this->mChannelFreeListHead.isEmpty()) {
                chan = reinterpret_cast<ChannelI*>(this->mChannelFreeListHead.getNext());
            } else {
                if (this->mChannelSortedListHead.isEmpty()) {
                    return FMOD_ERR_CHANNEL_ALLOC;
                }

                chan = reinterpret_cast<ChannelI*>(this->mChannelSortedListHead.getPrev()->getData());
                chan->stopEx(true, false, true, true, false, true, false);
            }
        }

        chan->removeNode();
        chan->addBefore(&this->mChannelUsedListHead);
    }

    result = this->mSoftware->getFreeChannel(FMOD_SOFTWARE, realchannel, 1, 1, nullptr);
    if (result != FMOD_OK) {
        result = this->mEmulated->getFreeChannel(FMOD_SOFTWARE, realchannel, 1, 1, nullptr);
    }
    if (result != FMOD_OK) {
        return result;
    }

    chan->mRealChannel[0] = realchannel[0];
    chan->mNumRealChannels = 1;

    return FMOD_OK;
}

FMOD_RESULT SystemI::findChannel(FMOD_CHANNELINDEX id, SoundI* sound, ChannelI** channel) {
    ChannelReal* realchannel[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { 0 };
    FMOD_MODE mode;
    FMOD_RESULT result;
    ChannelI* chan;
    uint32_t subchannels;
    int32_t found;

    if (!channel) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = sound->getMode(&mode);
    if (result != FMOD_OK) {
        return result;
    }

    if (sound->mNumSubSounds && mode & FMOD_CREATESTREAM) {
        return FMOD_ERR_SUBSOUNDS;
    }

    if (mode & FMOD_UNIQUE) {
        result = this->stopSound(sound);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (id == FMOD_CHANNEL_REUSE) {
        chan = *channel;

        if (!chan) {
            id = FMOD_CHANNEL_FREE;
        } else {
            if (!chan->mRealChannel[0] || chan->mRealChannel[0]->mFlags & CHANNELREAL_FLAG_STOPPED) {
                id = static_cast<FMOD_CHANNELINDEX>(chan->mIndex);
            } else {
                chan->stopEx(false, false, false, true, true, true, false);
            }
        }
    }

    if (id != FMOD_CHANNEL_REUSE) {
        *channel = nullptr;

        if (id != FMOD_CHANNEL_FREE) {
            chan = &this->mChannel[id];
            chan->stop();
        } else {
            if (!this->mChannelFreeListHead.isEmpty()) {
                chan = reinterpret_cast<ChannelI*>(this->mChannelFreeListHead.getNext());
            } else {
                if (this->mChannelSortedListHead.isEmpty()) {
                    return FMOD_ERR_CHANNEL_ALLOC;
                }

                chan = reinterpret_cast<ChannelI*>(this->mChannelSortedListHead.getPrev()->getData());
                chan->stopEx(true, false, true, true, true, true, false);
            }
        }

        chan->removeNode();
        chan->addBefore(&this->mChannelUsedListHead);
    }

    if (mode & FMOD_CREATESTREAM) {
        auto stream = reinterpret_cast<Stream*>(sound);
        if (stream->mSample) {
            subchannels = stream->mSample->mNumSubSamples;
        } else {
            subchannels = 0;
        }
    } else {
        auto sample = reinterpret_cast<Sample*>(sound);
        subchannels = sample->mNumSubSamples;
    }

    if (!subchannels) {
        subchannels = 1;
    }

    if ((mode & FMOD_SOFTWARE || this->mOutputType == FMOD_OUTPUTTYPE_OPENAL) && mode & FMOD_CREATECOMPRESSEDSAMPLE) {
        if (sound->mFormat == FMOD_SOUND_FORMAT_MPEG) {
            result = this->mDSPCodecPool_MPEG.areAnyFree();
        } else if (sound->mFormat == FMOD_SOUND_FORMAT_IMAADPCM) {
            result = this->mDSPCodecPool_ADPCM.areAnyFree();
        } else {
            result = FMOD_ERR_FORMAT;
        }
    }

    if (mode & FMOD_SOFTWARE || !mOutput->mDescription.createsample) {
        if (result == FMOD_OK) {
            result = this->mSoftware->getFreeChannel(mode, realchannel, subchannels, sound->mChannels, &found);
        }

    } else {
        result = this->mOutput->getFreeChannel(mode, realchannel, subchannels, sound->mChannels, &found);
    }

    if (result != FMOD_OK || found != subchannels) {
        subchannels = 1;
        result = this->mEmulated->getFreeChannel(mode, realchannel, subchannels, sound->mChannels, 0);
    }
    if (result != FMOD_OK) {
        return result;
    }

    if (realchannel[0]->mOutput == this->mEmulated) {
        chan->mFlags |= CHANNELI_FLAG_JUSTWENTVIRTUAL;
    }

    if (mode & FMOD_CREATESTREAM) {
        if (realchannel[0]->mOutput == this->mEmulated) {
            chan->mNumRealChannels = 1;
            chan->mRealChannel[0] = realchannel[0];
            chan->mRealChannel[0]->mSubChannelIndex = 0;
        } else {
            auto stream = reinterpret_cast<Stream*>(sound);
            auto channelstream = stream->mChannel;

            chan->mNumRealChannels = 1;
            chan->mRealChannel[0] = channelstream;

            channelstream->mNumSubChannels = subchannels;
            channelstream->mSubChannelIndex = 0;
            for (int32_t i = 0; i < subchannels; i++) {
                channelstream->mSubChannel[i] = realchannel[i];
                channelstream->mSubChannel[i]->mSubChannelIndex = i;
            }
        }
    } else {
        chan->mNumRealChannels = subchannels;
        for (int32_t i = 0; i < subchannels; i++) {
            chan->mRealChannel[i] = realchannel[i];
            chan->mRealChannel[i]->mSubChannelIndex = i;
        }
    }

    *channel = chan;

    return FMOD_OK;
}

FMOD_RESULT SystemI::flushDSPConnectionRequests(bool calledfrommainthread) {
    DSPConnectionRequest* request;

    FMOD_OS_CriticalSection_Enter(this->mDSPRequestCrit);

    if (this->mConnectionRequestFlushing != false) {
        FMOD_OS_CriticalSection_Leave(this->mDSPRequestCrit);
        return FMOD_OK;
    }

    this->mConnectionRequestFlushing = true;

    request = reinterpret_cast<DSPConnectionRequest*>(this->mConnectionRequestUsedHead.getNext());
    while (request != &this->mConnectionRequestUsedHead) {
        switch (request->mRequest) {
        case DSPCONNECTION_REQUEST_ADDINPUT:
            request->mThis->addInputInternal(request->mTarget, false, request->mConnection, nullptr);
            break;
        case DSPCONNECTION_REQUEST_DISCONNECTFROM:
            request->mThis->disconnectFromInternal(request->mTarget);
            break;

        case DSPCONNECTION_REQUEST_DISCONNECTALLINPUTS:
            request->mThis->disconnectAllInternal(true, false);
            break;

        case DSPCONNECTION_REQUEST_DISCONNECTALLOUTPUTS:
            request->mThis->disconnectAllInternal(false, true);
            break;

        case DSPCONNECTION_REQUEST_DISCONNECTALL:
            request->mThis->disconnectAllInternal(true, true);
            break;
        }

        request->removeNode();
        request->addBefore(&this->mConnectionRequestFreeHead);

        request = reinterpret_cast<DSPConnectionRequest*>(this->mConnectionRequestUsedHead.getNext());
    }

    this->mConnectionRequestFlushing = false;
    FMOD_OS_CriticalSection_Leave(this->mDSPRequestCrit);

    return FMOD_OK;
}

FMOD_RESULT SystemI::get3DListenerAttributes(int32_t listener, FMOD_VECTOR* pos, FMOD_VECTOR* vel, FMOD_VECTOR* forward, FMOD_VECTOR* up) {
    if (listener < 0 || listener >= LISTENER_MAX) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (pos) {
        *pos = this->mListener[listener].mPosition;
    }

    if (vel) {
        *vel = this->mListener[listener].mVelocity;
    }

    if (forward) {
        *forward = this->mListener[listener].mFront;
    }

    if (up) {
        *up = this->mListener[listener].mUp;
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::get3DNumListeners(int32_t* numlisteners) {
    if (!numlisteners) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numlisteners = this->mNumListeners;
    return FMOD_OK;
}

FMOD_RESULT SystemI::get3DReverbActive(bool* state) {
    if (state) {
        *state = this->mReverb3DActive;
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::get3DSettings(float* dopplerscale, float* distancescale, float* rolloffscale) {
    if (dopplerscale) {
        *dopplerscale = this->mDopplerScale;
    }
    if (distancescale) {
        *distancescale = this->mDistanceScale;
    }
    if (rolloffscale) {
        *rolloffscale = this->mRolloffScale;
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::getChannel(int32_t id, ChannelI** channel) {
    FMOD_UINT_NATIVE handle;

    if (!channel || id < 0 || id >= this->mNumChannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    handle = ((id & 0xFFF | (this->mIndex << 12)) << 16);
    *channel = reinterpret_cast<ChannelI*>(handle);

    return FMOD_OK;
}

FMOD_RESULT SystemI::getDSPBufferSize(uint32_t* bufferlength, int32_t* numbuffers) {
    if (bufferlength) {
        *bufferlength = this->mDSPBlockSize;
    }
    if (numbuffers) {
        *numbuffers = this->mDSPBufferSize / this->mDSPBlockSize;
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::getDSPHead(DSPI** dsp) {
    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (!this->mDSPSoundCard) {
        return FMOD_ERR_INTERNAL;
    }
    *dsp = this->mDSPSoundCard;
    return FMOD_OK;
}

FMOD_RESULT SystemI::getDriver(int32_t* driver) {
    if (!driver) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *driver = this->mSelectedDriver;

    return FMOD_OK;
}

FMOD_RESULT SystemI::getDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode) {
    FMOD_RESULT result;
    FMOD_CAPS lcaps;
    FMOD_SPEAKERMODE lcontrolpanelspeakermode;
    int32_t lminfrequency;
    int32_t lmaxfrequency;
    int32_t lnum2dchannels;
    int32_t lnum3dchannels;
    int32_t ltotalchannels;
    int32_t numdrivers;

    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    result = this->getNumDrivers(&numdrivers);
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0 || id >= numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mInitialized) {
        result = this->setOutput(this->mOutputType);
        if (result != FMOD_OK) {
            return result;
        }
    }

    lcaps = FMOD_CAPS_NONE;
    lminfrequency = 0;
    lmaxfrequency = 0;
    lnum2dchannels = 0;
    lnum3dchannels = 0;
    ltotalchannels = 0;
    lcontrolpanelspeakermode = FMOD_SPEAKERMODE_STEREO;

    if (this->mOutput->mDescription.getdrivercapsex2) {
        this->mOutput->readfrommixer = Output::mixCallback;
        result = this->mOutput->mDescription.getdrivercapsex2(this->mOutput, id, &lcaps, &lminfrequency, &lmaxfrequency, &lcontrolpanelspeakermode, &lnum2dchannels, &lnum3dchannels, &ltotalchannels);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (this->mOutput->mDescription.getdrivercapsex) {
        this->mOutput->readfrommixer = Output::mixCallback;
        result = this->mOutput->mDescription.getdrivercapsex(this->mOutput, id, &lcaps, &lminfrequency, &lmaxfrequency, &lcontrolpanelspeakermode);
        if (result != FMOD_OK) {
            return result;
        }
    } else if (this->mOutput->mDescription.getdrivercaps) {
        this->mOutput->readfrommixer = Output::mixCallback;
        result = this->mOutput->mDescription.getdrivercaps(this->mOutput, id, &lcaps);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mOutput->mNum2DChannelsFromCaps = lnum2dchannels;
    this->mOutput->mNum3DChannelsFromCaps = lnum3dchannels;
    this->mOutput->mTotalChannelsFromCaps = ltotalchannels;
    if (caps) {
        *caps = lcaps;
    }
    if (minfrequency) {
        *minfrequency = lminfrequency;
    }
    if (maxfrequency) {
        *maxfrequency = lmaxfrequency;
    }
    if (controlpanelspeakermode) {
        *controlpanelspeakermode = lcontrolpanelspeakermode;
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::getDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;
    int32_t numdrivers;

    result = this->getNumDrivers(&numdrivers);
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0 || id >= numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mInitialized) {
        result = this->setOutput(this->mOutputType);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (this->mOutput->mDescription.getdrivername) {
        this->mOutput->readfrommixer = Output::mixCallback;
        return this->mOutput->mDescription.getdrivername(this->mOutput, id, name, namelen);
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::getHardwareChannels(int32_t* num2d, int32_t* num3d, int32_t* total) {
    FMOD_RESULT result;
    int32_t numhw3d = 0;
    int32_t numhw2d = 0;
    int32_t numhwtotal;

    if (this->mInitialized) {
        if (this->mOutput) {
            if (this->mOutput->mChannelPool) {
                result = this->mOutput->mChannelPool->getNumChannels(&numhw2d);
                if (result != FMOD_OK) {
                    return result;
                }
            }

            if (this->mOutput->mChannelPool3D) {
                result = this->mOutput->mChannelPool3D->getNumChannels(&numhw3d);
                if (result != FMOD_OK) {
                    return result;
                }
            }
        }

        numhwtotal = numhw3d + numhw2d;
    } else {
        result = this->setOutput(this->mOutputType);
        if (result != FMOD_OK) {
            return result;
        }

        numhw2d = this->mOutput->mNum2DChannelsFromCaps;
        numhw3d = this->mOutput->mNum3DChannelsFromCaps;
        numhwtotal = this->mOutput->mTotalChannelsFromCaps;
    }

    if (num3d) {
        *num3d = numhw3d;
    }

    if (num2d) {
        *num2d = numhw2d;
    }

    if (total) {
        *total = numhwtotal;
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::getListenerObject(int32_t listener, Listener** listenerobject) {
    if (!listenerobject || listener < 0 || listener >= this->mNumListeners) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *listenerobject = &this->mListener[listener];
    return FMOD_OK;
}

FMOD_RESULT SystemI::getMasterChannelGroup(ChannelGroupI** channelgroup) {
    if (!channelgroup) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (!this->mChannelGroup) {
        *channelgroup = nullptr;
        return FMOD_ERR_UNINITIALIZED;
    }
    *channelgroup = this->mChannelGroup;
    return FMOD_OK;
}

FMOD_RESULT SystemI::getNumDrivers(int32_t* numdrivers) {
    FMOD_RESULT result;

    if (!numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mInitialized) {
        result = this->setOutput(this->mOutputType);
        if (result != FMOD_OK) {
            *numdrivers = 0;
            return result;
        }
    }

    if (this->mOutput->mDescription.getnumdrivers) {
        this->mOutput->readfrommixer = Output::mixCallback;
        return this->mOutput->mDescription.getnumdrivers(this->mOutput, numdrivers);
    }

    *numdrivers = 0;
    return FMOD_OK;
}

FMOD_RESULT SystemI::getOutput(FMOD_OUTPUTTYPE* output) {
    if (!output) {
        return FMOD_ERR_INVALID_PARAM;
    }
    *output = this->mOutputType;
    return FMOD_OK;
}

FMOD_RESULT SystemI::getRecordDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;
    int32_t numdrivers;

    if (!this->mOutput) {
        return FMOD_ERR_UNINITIALIZED;
    }

    result = this->getRecordNumDrivers(&numdrivers);
    if (result != FMOD_OK) {
        return result;
    }

    if (id < 0 || id >= numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mOutput->mDescription.record_getdriverinfo) {
        this->mOutput->readfrommixer = Output::mixCallback;
        return this->mOutput->mDescription.record_getdriverinfo(this->mOutput, id, name, namelen);
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::getRecordNumDrivers(int32_t* numdrivers) {
    if (!this->mOutput) {
        return FMOD_ERR_UNINITIALIZED;
    }
    if (!numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mOutput->mDescription.record_getnumdrivers) {
        this->mOutput->readfrommixer = Output::mixCallback;
        return this->mOutput->mDescription.record_getnumdrivers(this->mOutput, numdrivers);
    }

    *numdrivers = 0;
    return FMOD_OK;
}

FMOD_RESULT SystemI::getRecordPosition(uint32_t* position) {
    if (!this->mInitialized) {
        return FMOD_ERR_UNINITIALIZED;
    }
    *position = this->mOutput->mRecordOffset;
    return FMOD_OK;
}

FMOD_RESULT SystemI::getReverbAmbientProperties(FMOD_REVERB_PROPERTIES* prop) {
    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_memcpy(prop, &mReverb3DAmbientProperties, sizeof(FMOD_REVERB_PROPERTIES));
    return FMOD_OK;
}

FMOD_RESULT SystemI::getSoftwareFormat(int32_t* samplerate, FMOD_SOUND_FORMAT* format, int32_t* numoutputchannels, int32_t* maxinputchannels, FMOD_DSP_RESAMPLER* resamplemethod, int32_t* bits) {
    if (samplerate) {
        *samplerate = this->mOutputRate;
    }
    if (format) {
        *format = this->mOutputFormat;
    }
    if (numoutputchannels) {
        *numoutputchannels = this->mMaxOutputChannels;
    }
    if (maxinputchannels) {
        *maxinputchannels = this->mMaxInputChannels;
    }
    if (resamplemethod) {
        *resamplemethod = this->mResampleMethod;
    }
    if (bits) {
        SoundI::getBitsFromFormat(this->mOutputFormat, bits);
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::getSpeakerMode(FMOD_SPEAKERMODE* speakermode) {
    if (!speakermode) {
        return FMOD_ERR_INVALID_PARAM;
    }
    *speakermode = this->mSpeakerMode;
    return FMOD_OK;
}

FMOD_RESULT SystemI::getVersion(uint32_t* version) {
    if (!version) {
        return FMOD_ERR_INVALID_PARAM;
    }
    *version = FMOD_VERSION;
    return FMOD_OK;
}

FMOD_RESULT SystemI::init(int32_t maxchannels, FMOD_INITFLAGS flags, void* extradriverdata) {
    FMOD_RESULT result;
    FMOD_SPEAKERMODE speakerMode;
    int32_t channellimit = 0x1000;

    if (maxchannels < 0 || maxchannels >= channellimit) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->closeEx(true);
    if (result != FMOD_OK) {
        return result;
    }

    this->mFlags = flags;

    FMOD_OS_Thread_GetCurrentID(&this->mMainThreadID);

    result = this->setOutput(this->mOutputType);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mOutput->mDescription.initex) {
        this->mOutput->readfrommixer = Output::mixCallback;

        speakerMode = this->mSpeakerMode;

        result = this->mOutput->mDescription.initex(
            this->mOutput,
            this->mSelectedDriver,
            flags,
            &this->mOutputRate,
            this->mMaxOutputChannels,
            &this->mOutputFormat,
            &speakerMode,
            this->mDSPBlockSize,
            this->mDSPBufferSize / this->mDSPBlockSize,
            0,
            0,
            extradriverdata);
        if (result != FMOD_OK) {
            return result;
        }
        if (speakerMode != this->mSpeakerMode) {
            this->setSpeakerMode(speakerMode);
        }
    } else if (this->mOutput->mDescription.init) {
        this->mOutput->readfrommixer = Output::mixCallback;

        result = this->mOutput->mDescription.init(
            this->mOutput,
            this->mSelectedDriver,
            flags,
            &this->mOutputRate,
            this->mMaxOutputChannels,
            &this->mOutputFormat,
            this->mDSPBlockSize,
            this->mDSPBufferSize / this->mDSPBlockSize,
            extradriverdata);

        if (result != FMOD_OK) {
            return result;
        }
    }

    if (!(this->mFlags & FMOD_INIT_DISABLESOFTWARE)) {
        FMOD_DSP_DESCRIPTION description;
        FMOD_DSP_DESCRIPTION_EX descriptionex;
        FMOD_SOUND_FORMAT bufferformat;
        int32_t bufferchannels;

        this->mSoftware = FMOD_Object_Alloc(OutputSoftware);
        if (!this->mSoftware) {
            return FMOD_ERR_MEMORY;
        }

        this->mSoftware->mSystem = this;

        if (!this->mDSPCrit) {
            result = FMOD_OS_CriticalSection_Create(&this->mDSPCrit);
            if (result != FMOD_OK) {
                return result;
            }
        }

        if (!this->mDSPLockCrit) {
            result = FMOD_OS_CriticalSection_Create(&this->mDSPLockCrit);
            if (result != FMOD_OK) {
                return result;
            }
        }

        if (!this->mDSPConnectionCrit) {
            result = FMOD_OS_CriticalSection_Create(&this->mDSPConnectionCrit);
            if (result != FMOD_OK) {
                return result;
            }
        }

        if (!this->mDSPRequestCrit) {
            result = FMOD_OS_CriticalSection_Create(&this->mDSPRequestCrit);
            if (result != FMOD_OK) {
                return result;
            }
        }

        this->mConnectionRequestUsedHead.initNode();
        this->mConnectionRequestFreeHead.initNode();

        for (int32_t i = 0; i < FMOD_DSP_CONNECTION_REQUEST_MAX; i++) {
            auto request = &this->mConnectionRequest[i];
            request->initNode();
            request->addBefore(&this->mConnectionRequestFreeHead);
        }

        result = this->getSoftwareFormat(nullptr, &bufferformat, &bufferchannels, nullptr, nullptr, nullptr);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mMaxInputChannels < bufferchannels) {
            this->mMaxInputChannels = bufferchannels;
        }

        for (int32_t i = 0; i < 2; i++) {
            this->mDSPTempBuffMem[i] = reinterpret_cast<float*>(FMOD_Memory_Calloc((this->mDSPBlockSize * (bufferchannels < this->mMaxInputChannels ? this->mMaxInputChannels : bufferchannels) * sizeof(float)) + 16));
            if (!this->mDSPTempBuffMem[i]) {
                return FMOD_ERR_MEMORY;
            }
            this->mDSPTempBuff[i] = reinterpret_cast<float*>(FMOD_ALIGNPOINTER(this->mDSPTempBuffMem[i], 16));
        }

        result = this->mDSPConnectionPool.init(this, 1 + (this->mNumSoftwareChannels * 2), bufferchannels < 2 ? 2 : bufferchannels, this->mMaxInputChannels);
        if (result != FMOD_OK) {
            return result;
        }

        FMOD_memset(&descriptionex, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));
        FMOD_strcpy(descriptionex.name, "FMOD SoundCard Unit");
        descriptionex.version = 0x10100;
        descriptionex.channels = bufferchannels;
        descriptionex.create = nullptr;
        descriptionex.release = nullptr;
        descriptionex.read = nullptr;
        descriptionex.setposition = nullptr;
        descriptionex.mCategory = FMOD_DSP_CATEGORY_SOUNDCARD;
        descriptionex.mFormat = bufferformat;
        result = this->createDSP(&descriptionex, &this->mDSPSoundCard, true);
        if (result != FMOD_OK) {
            return result;
        }

        this->mDSPSoundCard->setActive(true);

        FMOD_memset(&description, 0, sizeof(FMOD_DSP_DESCRIPTION));
        FMOD_strcpy(description.name, "FMOD ChannelGroup Target Unit");
        description.version = 0x00010100;
        description.channels = 0;
        description.create = nullptr;
        description.release = nullptr;
        description.read = nullptr;
        description.setposition = nullptr;
        result = this->createDSP(&description, &this->mDSPChannelGroupTarget);
        if (result != FMOD_OK) {
            return result;
        }

        this->mDSPChannelGroupTarget->setDefaults(static_cast<float>(this->mOutputRate), -1.0, -1.0, -1);
        this->mDSPChannelGroupTarget->setActive(true);

        result = this->getSpeakerMode(&speakerMode);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mDSPSoundCard->addInput(this->mDSPChannelGroupTarget);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->createChannelGroup("FMOD master group", &this->mChannelGroup);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->createSoundGroup("FMOD master group", &this->mSoundGroup);
    if (result != FMOD_OK) {
        return result;
    }

    if (!(this->mFlags & FMOD_INIT_DISABLESOFTWARE)) {
        result = this->mSoftware->init(this->mNumSoftwareChannels);
        if (result != FMOD_OK) {
            return result;
        }

        if (this->mOutput->mDescription.start) {
            this->mOutput->readfrommixer = Output::mixCallback;
            result = this->mOutput->mDescription.start(this->mOutput);
            if (result != FMOD_OK) {
                return result;
            }
        } else if (this->mOutput->mDescription.polling) {
            auto outputpolled = reinterpret_cast<OutputPolled*>(this->mOutput);
            result = outputpolled->start();
            if (result != FMOD_OK) {
                return result;
            }
        }
    }

    if (maxchannels > 0) {
        this->mEmulated = FMOD_Object_Alloc(OutputEmulated);
        if (!this->mEmulated) {
            return FMOD_ERR_MEMORY;
        }

        this->mEmulated->mSystem = this;
        result = this->mEmulated->init(maxchannels);
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mNumChannels = maxchannels;

    if (maxchannels) {
        this->mChannel = reinterpret_cast<ChannelI*>(FMOD_Memory_Calloc(sizeof(ChannelI) * maxchannels));
        if (!this->mChannel) {
            return FMOD_ERR_MEMORY;
        }

        for (int32_t i = 0; i < this->mNumChannels; i++) {
            new (&this->mChannel[i]) ChannelI(i, this);

            this->mChannel[i].addAfter(&this->mChannelFreeListHead);
            this->mChannel[i].setChannelGroup(this->mChannelGroup);
        }
    }

    if (!gSoundListCrit) {
        result = FMOD_OS_CriticalSection_Create(&gSoundListCrit);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (!this->mReadDataCrit) {
        result = FMOD_OS_CriticalSection_Create(&this->mReadDataCrit);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (!this->mMultiSubSampleLockBufferCrit) {
        result = FMOD_OS_CriticalSection_Create(&this->mMultiSubSampleLockBufferCrit);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (!this->mStreamThreadActive && !(this->mFlags & FMOD_INIT_STREAM_FROM_UPDATE)) {
        result = FMOD_OS_CriticalSection_Create(&this->mStreamRealchanCrit);
        if (result != FMOD_OK) {
            return result;
        }

        result = FMOD_OS_CriticalSection_Create(&this->mStreamUpdateCrit);
        if (result != FMOD_OK) {
            return result;
        }

        result = FMOD_OS_CriticalSection_Create(&this->mStreamListCrit);
        if (result != FMOD_OK) {
            return result;
        }

        result = this->mStreamThread.initThread(
            "FMOD stream thread",
            streamThread, this,
            Thread::PRIORITY_VERYHIGH,
            nullptr,
            0x8000,
            false,
            10);
        if (result != FMOD_OK) {
            return result;
        }

        this->mStreamThreadActive = true;
    }

    if (!File::gFileCrit) {
        result = FMOD_OS_CriticalSection_Create(&File::gFileCrit);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (!(this->mFlags & FMOD_INIT_DISABLESOFTWARE)) {
        if (this->mAdvancedSettings.maxADPCMcodecs) {
            result = this->mDSPCodecPool_ADPCM.init(FMOD_DSP_CATEGORY_DSPCODECADPCM, 64, this->mAdvancedSettings.maxADPCMcodecs);
            if (result != FMOD_OK) {
                return result;
            }

            for (int32_t i = 0; i < this->mDSPCodecPool_ADPCM.mNumDSPCodecs; i++) {
                auto wav = reinterpret_cast<CodecWav*>(this->mDSPCodecPool_ADPCM.mPool[i]->mCodec);

                wav->mSrcFormat = &wav->mSrcFormatMemory;
                wav->mReadBuffer = this->mDSPCodecPool_ADPCM.mReadBuffer;
                wav->mSrcFormat->Format.wFormatTag = WAVE_FORMAT_IMA_ADPCM;
            }
        }

        if (this->mAdvancedSettings.maxMPEGcodecs) {
            result = this->mDSPCodecPool_MPEG.init(FMOD_DSP_CATEGORY_DSPCODECMPEG, 1152, this->mAdvancedSettings.maxMPEGcodecs);
            if (result != FMOD_OK) {
                return result;
            }

            for (int32_t i = 0; i < this->mDSPCodecPool_MPEG.mNumDSPCodecs; i++) {
                auto mpeg = reinterpret_cast<CodecMPEG*>(this->mDSPCodecPool_MPEG.mPool[i]->mCodec);

                mpeg->mSrcDataOffset = 0;
                mpeg->mWaveFormatMemory = nullptr;
                mpeg->reset();
            }
        }
    }

    FMOD_REVERB_PROPERTIES prop = FMOD_PRESET_OFF;

    result = this->mReverbGlobal.init(this, false, FMOD_REVERB_PHYSICAL);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->setReverbProperties(&prop, false);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mReverb3D.init(this, false, FMOD_REVERB_PHYSICAL);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->set3DReverbProperties(&prop, false);
    if (result != FMOD_OK) {
        return result;
    }

    // if (this->mFlags & FMOD_INIT_ENABLE_DSPNET) {
    //     this->mDSPNet.init();
    // }

    this->mSpeakerLevelsPool.mSystem = this;

    this->mInitialized = true;
    gGlobal->gSystemInitCount++;

    return FMOD_OK;
}

FMOD_RESULT SystemI::isRecording(bool* recording) {
    if (this->mInitialized == false) {
        return FMOD_ERR_UNINITIALIZED;
    }

    if (!recording) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *recording = this->mOutput->mRecording;
    return FMOD_OK;
}

FMOD_RESULT SystemI::lockDSP() {
    FMOD_OS_CriticalSection_Enter(this->mDSPLockCrit);
    return FMOD_OK;
}

FMOD_RESULT SystemI::playDSP(FMOD_CHANNELINDEX channelid, DSPI* dsp, bool paused, ChannelI** channel) {
    FMOD_RESULT result;
    ChannelI* chan = nullptr;

    if (!dsp) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (channel) {
        if (channelid == FMOD_CHANNEL_REUSE) {
            ChannelI::validate(reinterpret_cast<Channel*>(*channel), &chan);
        }
        if (channel) {
            *channel = nullptr;
        }
    }

    result = this->findChannel(channelid, dsp, &chan);
    if (result != FMOD_OK) {
        return result;
    }

    result = chan->play(dsp, paused, true, false);
    if (result != FMOD_OK) {
        chan->stopEx(false, true, true, true, false, true, false);
        return result;
    }

    result = chan->updatePosition();
    if (result != FMOD_OK) {
        return result;
    }

    if (channelid == FMOD_CHANNEL_REUSE && *channel) {
        chan->mHandleCurrent = chan->mHandleOriginal;
    } else {
        result = chan->referenceStamp(true);
        if (result != FMOD_OK) {
            if (channel) {
                *channel = nullptr;
            }
            return result;
        }
    }

    if (channel) {
        // XXX: verify that this works on AMD64
        *channel = reinterpret_cast<ChannelI*>(chan->mHandleCurrent);
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::playSound(FMOD_CHANNELINDEX channelid, SoundI* sound, bool paused, ChannelI** channel) {
    FMOD_RESULT result;
    ChannelI* chan = nullptr;
    bool mute = false;

    if (channel && channelid == FMOD_CHANNEL_REUSE) {
        ChannelI::validate(reinterpret_cast<Channel*>(*channel), &chan);
    }

    if (!sound) {
        if (channel) {
            *channel = nullptr;
        }
        return FMOD_ERR_INVALID_PARAM;
    }

    if (sound->mOpenState != FMOD_OPENSTATE_READY) {
        if (channel) {
            *channel = nullptr;
        }
        return FMOD_ERR_NOTREADY;
    }

    if (sound->mType == FMOD_SOUND_TYPE_PLAYLIST) {
        return FMOD_ERR_FORMAT;
    }

    if (sound->mSoundGroup && sound->mSoundGroup->mMaxAudible >= 0) {
        int32_t numplaying;
        result = sound->mSoundGroup->getNumPlaying(&numplaying);
        if (result != FMOD_OK) {
            return result;
        }

        if (numplaying >= sound->mSoundGroup->mMaxAudible) {
            switch (sound->mSoundGroup->mMaxAudibleBehavior) {
            case FMOD_SOUNDGROUP_BEHAVIOR_FAIL: {
                return FMOD_ERR_MAXAUDIBLE;
            }
            case FMOD_SOUNDGROUP_BEHAVIOR_MUTE: {
                mute = true;
                break;
            }
            case FMOD_SOUNDGROUP_BEHAVIOR_STEALLOWEST: {
                auto current = this->mChannelSortedListHead.getNext();
                while (current != &this->mChannelSortedListHead) {
                    auto channel = reinterpret_cast<ChannelI*>(current->getData());
                    if (channel->mRealChannel[0] && channel->mRealChannel[0]->mSound) {
                        auto soundgroup = channel->mRealChannel[0]->mSound->mSoundGroup;
                        if (soundgroup == sound->mSoundGroup) {
                            channelid = FMOD_CHANNEL_REUSE;
                            chan = channel;
                        }
                    }
                    current = current->getNext();
                }
                break;
            }
            default: {
                break;
            }
            }
        }
    }

    result = this->findChannel(channelid, sound, &chan);
    if (result != FMOD_OK) {
        if (channel) {
            *channel = nullptr;
        }
        return result;
    }

    result = chan->play(sound, paused, true, mute);
    if (result != FMOD_OK) {
        if (channel) {
            *channel = nullptr;
        }
        chan->stopEx(false, true, true, true, false, true, false);
        return result;
    }

    result = chan->updatePosition();
    if (result != FMOD_OK) {
        if (channel) {
            *channel = nullptr;
        }
        return result;
    }

    if (channelid == FMOD_CHANNEL_REUSE && *channel) {
        chan->mHandleCurrent = chan->mHandleOriginal;
    } else {
        result = chan->referenceStamp(true);
        if (result != FMOD_OK) {
            if (channel) {
                *channel = nullptr;
            }
            return result;
        }
    }

    if (channel) {
        *channel = reinterpret_cast<ChannelI*>(chan->mHandleCurrent);
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::recordStart(SoundI* sound, bool loop) {
    FMOD_RESULT result;

    if (!this->mInitialized) {
        return FMOD_ERR_UNINITIALIZED;
    }

    if (!sound || sound->mMode & FMOD_CREATESTREAM) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->recordStop();

    this->mOutput->mRecordSound = sound;
    this->mOutput->mRecordOffset = 0;
    this->mOutput->mRecordLoop = loop;
    this->mOutput->mRecordLastCursorPos = 0;
    this->mOutput->mRecordBufferLength = 0;
    this->mOutput->mRecordRate = static_cast<int32_t>(sound->mDefaultFrequency);

    if (this->mOutput->mDescription.record_start) {
        this->mOutput->readfrommixer = Output::mixCallback;
        result = this->mOutput->mDescription.record_start(this->mOutput, this->mRecordID, reinterpret_cast<FMOD_SOUND*>(sound), loop);
        if (result != FMOD_OK) {
            return result;
        }
    }

    // TODO: this may be more cleanly implemented using SoundI::getBytesFromSamples
    this->mOutput->mRecordTempBufferLength = 2048;
    this->mOutput->mRecordTempBuffer = reinterpret_cast<float*>(FMOD_Memory_Calloc(sound->mChannels * 8192));

    if (!this->mOutput->mRecordTempBuffer) {
        return FMOD_ERR_MEMORY;
    }

    this->mOutput->mRecordResample = false;

    if (this->mOutput->mRecordRate != sound->mDefaultFrequency) {
        uint32_t blocklength = static_cast<int32_t>(0.01f * this->mOutput->mRecordRate);
        blocklength /= 16;
        blocklength *= 16;

        FMOD_DSP_DESCRIPTION_EX description;
        memset(&description, 0, sizeof(FMOD_DSP_DESCRIPTION_EX));
        description.channels = sound->mChannels;
        description.userdata = this->mOutput;
        description.mFormat = FMOD_SOUND_FORMAT_PCMFLOAT;
        description.read = Output::recordResamplerReadCallback;
        description.mResamplerBlockLength = blocklength;

        this->mOutput->mRecordResamplerDSP.mSystem = this;
        this->mOutput->mRecordResamplerDSP.alloc(&description);
        this->mOutput->mRecordResamplerDSP.mTargetFrequency = static_cast<int32_t>(sound->mDefaultFrequency);

        this->mOutput->mRecordResamplerDSP.setFrequency(this->mOutput->mRecordRate);
        this->mOutput->mRecordResample = true;
    }

    this->mOutput->mRecording = true;
    return FMOD_OK;
}

FMOD_RESULT SystemI::recordStop() {
    FMOD_RESULT result = FMOD_OK;

    if (this->mInitialized == false) {
        return FMOD_ERR_UNINITIALIZED;
    }

    FMOD_OS_CriticalSection_Enter(this->mDSPLockCrit);
    this->mOutput->mRecording = false;
    this->mOutput->mRecordSound = nullptr;
    FMOD_OS_CriticalSection_Leave(this->mDSPLockCrit);

    if (this->mOutput->mDescription.record_stop) {
        this->mOutput->readfrommixer = Output::mixCallback;
        result = this->mOutput->mDescription.record_stop(this->mOutput);
    }

    if (this->mOutput->mRecordTempBuffer) {
        FMOD_Memory_Free(this->mOutput->mRecordTempBuffer);
        this->mOutput->mRecordTempBuffer = nullptr;
        this->mOutput->mRecordTempBufferLength = 0;
    }

    return result;
}

FMOD_RESULT SystemI::release() {
    FMOD_RESULT result;
    if (this->mInitialized) {
        result = this->closeEx(false);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (this->mPluginFactory) {
        this->mPluginFactory->release();
        this->mPluginFactory = nullptr;
    }

    this->removeNode();

    FMOD_Memory_Free(this);

    return FMOD_OK;
}

FMOD_RESULT SystemI::set3DListenerAttributes(int32_t listener, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel, const FMOD_VECTOR* forward, const FMOD_VECTOR* up) {
    if (listener < 0 || listener >= LISTENER_MAX) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (forward) {
        if (this->mListener[listener].mLastFront.x != forward->x ||
            this->mListener[listener].mLastFront.y != forward->y ||
            this->mListener[listener].mLastFront.z != forward->z) {
            this->mListener[listener].mRotated = true;
        }

        this->mListener[listener].mLastFront = this->mListener[listener].mFront;
        this->mListener[listener].mFront = *forward;
    }

    if (up) {
        if (this->mListener[listener].mLastUp.x != up->x ||
            this->mListener[listener].mLastUp.y != up->y ||
            this->mListener[listener].mLastUp.z != up->z) {
            this->mListener[listener].mRotated = true;
        }

        this->mListener[listener].mLastUp = this->mListener[listener].mUp;
        this->mListener[listener].mUp = *up;
    }

    if (pos) {
        if (this->mListener[listener].mLastPosition.x != pos->x ||
            this->mListener[listener].mLastPosition.y != pos->y ||
            this->mListener[listener].mLastPosition.z != pos->z) {
            this->mListener[listener].mMoved = true;
        }

        this->mListener[listener].mPosition = *pos;
        this->mListener[listener].mLastPosition = this->mListener[listener].mPosition;
    }

    if (vel) {
        if (this->mListener[listener].mLastVelocity.x != vel->x ||
            this->mListener[listener].mLastVelocity.y != vel->y ||
            this->mListener[listener].mLastVelocity.z != vel->z) {
            this->mListener[listener].mMoved = true;
        }

        this->mListener[listener].mLastVelocity = this->mListener[listener].mVelocity;
        this->mListener[listener].mVelocity = *vel;
    }

    auto l_up = this->mListener[listener].mUp;
    auto l_front = this->mListener[listener].mFront;

    if (this->mFlags & FMOD_INIT_3D_RIGHTHANDED) {
        l_up.z = -l_up.z;
        l_front.z = -l_front.z;
    }

    FMOD_Vector_CrossProduct(&l_up, &l_front, &this->mListener[listener].mRight);

    return FMOD_OK;
}

FMOD_RESULT SystemI::set3DReverbActive(bool state) {
    this->mReverb3DActive = state;
    return FMOD_OK;
}

FMOD_RESULT SystemI::set3DReverbProperties(FMOD_REVERB_PROPERTIES* prop, bool force_create) {
    FMOD_RESULT result;
    FMOD_REVERB_CHANNELPROPERTIES cprops;

    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if ((!this->mReverb3D.mDSP) && (force_create || (prop->Environment != -1))) {
        result = this->mReverb3D.createDSP();
        if (result != FMOD_OK) {
            return result;
        }

        if (!this->mDSPChannelGroupTarget) {
            return FMOD_ERR_UNINITIALIZED;
        }

        result = this->mDSPChannelGroupTarget->addInput(this->mReverb3D.mDSP);
        if (result != FMOD_OK) {
            return result;
        }

        this->mReverb3D.setGain(1.0f);

        auto current = reinterpret_cast<ChannelI*>(this->mChannelUsedListHead.getNext());
        while (current != &this->mChannelUsedListHead) {
            result = current->getReverbProperties(&cprops);
            if (result != FMOD_OK) {
                return result;
            }

            result = current->setReverbProperties(&cprops);
            if (result != FMOD_OK) {
                return result;
            }

            current = reinterpret_cast<ChannelI*>(current->getNext());
        }

        this->mReverb3D.mDSP->setActive(true);
    }

    result = this->mReverb3D.setProperties(prop);
    return result;
}

FMOD_RESULT SystemI::set3DRolloffCallback(FMOD_3D_ROLLOFFCALLBACK callback) {
    this->mRolloffCallback = callback;
    return FMOD_OK;
}

FMOD_RESULT SystemI::set3DSettings(float dopplerscale, float distancescale, float rolloffscale) {
    if (dopplerscale < 0.0f) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (distancescale <= 0.0f) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (rolloffscale < 0.0f) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mDopplerScale = dopplerscale;
    this->mDistanceScale = distancescale;
    this->mRolloffScale = rolloffscale;
    return FMOD_OK;
}

FMOD_RESULT SystemI::set3DSpeakerPosition(FMOD_SPEAKER speaker, float x, float y, bool active) {
    if (speaker >= FMOD_SPEAKER_MAX) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mSpeaker[speaker].mSpeaker = speaker;
    this->mSpeaker[speaker].mPosition.x = x;
    this->mSpeaker[speaker].mPosition.y = 0.0f;
    this->mSpeaker[speaker].mPosition.z = y;
    this->mSpeaker[speaker].mActive = active;
    this->mSpeaker[speaker].mXZAngle = FMOD_Cart2Angle(static_cast<int32_t>(y * 256.0f), static_cast<int32_t>(x * 256.0f));

    return this->sortSpeakerList();
}

FMOD_RESULT SystemI::setCallback(FMOD_SYSTEM_CALLBACKTYPE type, FMOD_SYSTEM_CALLBACK callback) {
    if (type == FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED) {
        this->mCallback[FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED] = callback;
        return FMOD_OK;
    }
    return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT SystemI::setDSPBufferSize(uint32_t bufferlength, int32_t numbuffers) {
    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    if (!bufferlength || numbuffers < 2) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mDSPBlockSize = bufferlength;
    this->mDSPBufferSize = bufferlength * numbuffers;
    return FMOD_OK;
}

FMOD_RESULT SystemI::setDriver(int32_t driver) {
    FMOD_RESULT result;
    int32_t numdrivers;

    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    result = this->getNumDrivers(&numdrivers);
    if (result != FMOD_OK) {
        return result;
    }

    if (driver < -1 || driver >= numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mSelectedDriver = driver != -1 ? driver : 0;

    return FMOD_OK;
}

FMOD_RESULT SystemI::setHardwareChannels(int32_t min2d, int32_t max2d, int32_t min3d, int32_t max3d) {
    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    if (min2d < 0 || max2d < 0 || min3d < 0 || max3d < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mMinHardwareChannels2D = min2d;
    this->mMaxHardwareChannels2D = max2d;
    this->mMaxHardwareChannels3D = max3d;
    this->mMinHardwareChannels3D = min3d;

    return FMOD_OK;
}

FMOD_RESULT SystemI::setOutput(FMOD_OUTPUTTYPE outputtype) {
    FMOD_RESULT result;
    int32_t numoutputs;

    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    if (this->mOutput) {
        if (outputtype == this->mOutputType) {
            return FMOD_OK;
        }
        FMOD_Memory_Free(this->mOutput);
    }

    if (!this->mPluginsLoaded) {
        result = this->setUpPlugins();
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->mPluginFactory->getNumOutputs(&numoutputs);
    if (result != FMOD_OK) {
        return result;
    }

    if (outputtype == FMOD_OUTPUTTYPE_AUTODETECT) {
        FMOD_OS_Output_GetDefault(&outputtype);
    }

    for (int32_t i = 0; i < numoutputs; i++) {
        FMOD_OUTPUT_DESCRIPTION_EX* outputdesc = nullptr;

        result = this->mPluginFactory->getOutput(i, &outputdesc);
        if (result != FMOD_OK) {
            continue;
        }

        if (outputdesc->mType == outputtype) {
            result = this->mPluginFactory->createOutput(outputdesc, &this->mOutput);
            if (result != FMOD_OK) {
                return result;
            }

            this->mOutputHandle = i;
            this->mOutputType = this->mOutput->mDescription.mType;
            return FMOD_OK;
        }
    }

    return FMOD_ERR_PLUGIN_MISSING;
}

FMOD_RESULT SystemI::setRecordDriver(int32_t recorddriver) {
    FMOD_RESULT result;
    int32_t numdrivers;

    result = this->getRecordNumDrivers(&numdrivers);
    if (result != FMOD_OK) {
        return result;
    }

    if (recorddriver < -1 || recorddriver >= numdrivers) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mRecordID = recorddriver == -1 ? 0 : recorddriver;

    return FMOD_OK;
}

FMOD_RESULT SystemI::setReverbAmbientProperties(FMOD_REVERB_PROPERTIES* prop) {
    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (prop->Environment != -1) {
        this->set3DReverbActive(true);
    }

    FMOD_memcpy(&this->mReverb3DAmbientProperties, prop, sizeof(FMOD_REVERB_PROPERTIES));

    return FMOD_OK;
}

FMOD_RESULT SystemI::setReverbProperties(FMOD_REVERB_PROPERTIES* prop, bool force_create) {
    FMOD_RESULT result;

    if (!prop) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if ((!this->mReverbGlobal.mDSP) && (force_create || (prop->Environment != -1))) {
        result = this->mReverbGlobal.createDSP();
        if (result != FMOD_OK) {
            return result;
        }

        if (!this->mDSPChannelGroupTarget) {
            return FMOD_ERR_UNINITIALIZED;
        }

        result = this->mDSPChannelGroupTarget->addInput(this->mReverbGlobal.mDSP);
        if (result != FMOD_OK) {
            return result;
        }

        this->mReverbGlobal.setGain(1.0f);

        auto channel = reinterpret_cast<ChannelI*>(this->mChannelUsedListHead.getNext());
        while (channel != &this->mChannelUsedListHead) {
            FMOD_REVERB_CHANNELPROPERTIES cprops;

            result = channel->getReverbProperties(&cprops);
            if (result != FMOD_OK) {
                return result;
            }

            result = channel->setReverbProperties(&cprops);
            if (result != FMOD_OK) {
                return result;
            }

            channel = reinterpret_cast<ChannelI*>(channel->getNext());
        }

        this->mReverbGlobal.mDSP->setActive(true);
    }

    result = this->mReverbGlobal.setProperties(prop);
    return result;
}

FMOD_RESULT SystemI::setSoftwareChannels(int32_t numsoftwarechannels) {
    if (numsoftwarechannels < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    this->mNumSoftwareChannels = numsoftwarechannels;

    return FMOD_OK;
}

FMOD_RESULT SystemI::getSoftwareChannels(int32_t* numsoftwarechannels) {
    if (!numsoftwarechannels) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numsoftwarechannels = this->mNumSoftwareChannels;

    return FMOD_OK;
}

FMOD_RESULT SystemI::setSoftwareFormat(int32_t samplerate, FMOD_SOUND_FORMAT format, int32_t numoutputchannels, int32_t maxinputchannels, FMOD_DSP_RESAMPLER resamplermethod) {
    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    if (samplerate < 8000) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mOutputRate = samplerate;
    this->mOutputFormat = format;
    this->mResampleMethod = resamplermethod;

    if (numoutputchannels) {
        this->mMaxOutputChannels = numoutputchannels;
        this->mSpeakerMode = FMOD_SPEAKERMODE_RAW;
    }

    if (maxinputchannels > 0) {
        this->mMaxInputChannels = maxinputchannels;
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::setSpeakerMode(FMOD_SPEAKERMODE speakermode) {
    if (this->mInitialized) {
        return FMOD_ERR_INITIALIZED;
    }

    this->mSpeakerMode = speakermode;

    switch (this->mSpeakerMode) {
    case FMOD_SPEAKERMODE_RAW:
        return FMOD_OK;

    case FMOD_SPEAKERMODE_MONO:
        this->mMaxOutputChannels = 1;
        break;

    case FMOD_SPEAKERMODE_STEREO:
        this->mMaxOutputChannels = 2;
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_LEFT, -1.0f, 0.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_RIGHT, 1.0f, 0.0f, true);
        break;

    case FMOD_SPEAKERMODE_QUAD:
        this->mMaxOutputChannels = 4;
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_LEFT, -1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_RIGHT, 1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_LEFT, -1.0f, -1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_RIGHT, 1.0f, -1.0f, true);
        break;

    case FMOD_SPEAKERMODE_SURROUND:
        this->mMaxOutputChannels = 5;
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_LEFT, -1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_RIGHT, 1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_CENTER, 0.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_LEFT, -1.0f, -1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_RIGHT, 1.0f, -1.0f, true);
        break;

    case FMOD_SPEAKERMODE_5POINT1:
        this->mMaxOutputChannels = 6;
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_LEFT, -1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_RIGHT, 1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_CENTER, 0.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_LEFT, -1.0f, -1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_RIGHT, 1.0f, -1.0f, true);
        break;

    case FMOD_SPEAKERMODE_7POINT1:
        this->mMaxOutputChannels = 8;
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_LEFT, -1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_RIGHT, 1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_CENTER, 0.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_LEFT, -1.0f, -1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_RIGHT, 1.0f, -1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_SIDE_LEFT, -1.0f, 0.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_SIDE_RIGHT, 1.0f, 0.0f, true);
        break;

    case FMOD_SPEAKERMODE_PROLOGIC:
        this->mMaxOutputChannels = 2;
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_LEFT, -1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_RIGHT, 1.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_FRONT_CENTER, 0.0f, 1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_LEFT, -1.0f, -1.0f, true);
        this->set3DSpeakerPosition(FMOD_SPEAKER_BACK_RIGHT, 1.0f, -1.0f, true);
        break;

    default:
        break;
    }

    return this->sortSpeakerList();
}

FMOD_RESULT SystemI::setUpPlugins() {
    FMOD_RESULT result;

    this->mPluginFactory = FMOD_Object_Alloc(PluginFactory);
    if (!this->mPluginFactory) {
        return FMOD_ERR_MEMORY;
    }

    result = this->mPluginFactory->setSystem(this);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mPluginFactory->setPluginPath(this->mPluginPath);
    if (result != FMOD_OK) {
        return result;
    }

    FMOD_OS_Output_Register(this->mPluginFactory);

    this->mPluginFactory->registerOutput(OutputWavWriter::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerOutput(OutputWavWriter_NRT::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerOutput(OutputNoSound::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerOutput(OutputNoSound_NRT::getDescriptionEx(), nullptr, nullptr);

    this->mPluginFactory->registerCodec(CodecTag::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecCDDA::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecFSB::getDescriptionEx(), nullptr, &this->mFSBPluginHandle);
    // this->mPluginFactory->registerCodec(CodecVAG::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerCodec(CodecWav::getDescriptionEx(), nullptr, &this->mWAVPluginHandle);
    this->mPluginFactory->registerCodec(CodecOggVorbis::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecAIFF::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecFLAC::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecMOD::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecS3M::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecXM::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecIT::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecMIDI::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecDLS::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecASF::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerCodec(CodecPlaylist::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerCodec(CodecMPEG::getDescriptionEx(), nullptr, &this->mMPEGPluginHandle);
    // this->mPluginFactory->registerCodec(CodecRaw::getDescriptionEx(), nullptr, nullptr);

    this->mPluginFactory->registerDSP(DSPOscillator::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPLowPass::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPLowPass2::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPLowPassSimple::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPHighPass::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPEcho::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPFlange::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPDistortion::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPNormalize::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPParamEq::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPPitchShift::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPChorus::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerDSP(DSPReverb::getDescriptionEx(), nullptr, nullptr);
    // this->mPluginFactory->registerDSP(DSPITEcho::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPCompressor::getDescriptionEx(), nullptr, nullptr);
    this->mPluginFactory->registerDSP(DSPSfxReverb::getDescriptionEx(), nullptr, nullptr);

    // this->mPluginFactory->registerCodec(CodecUser::getDescriptionEx(), nullptr, nullptr);

    this->mPluginsLoaded = true;

    return FMOD_OK;
}

FMOD_RESULT SystemI::sortSpeakerList() {
    bool used[FMOD_SPEAKER_MAX];
    int32_t outputchannels;

    if (this->mSpeakerMode == FMOD_SPEAKERMODE_RAW) {
        return FMOD_OK;
    }

    for (int32_t i = 0; i < FMOD_SPEAKER_MAX; i++) {
        this->mSpeakerList[i] = nullptr;
    }
    FMOD_memset(used, 0, sizeof(bool) * FMOD_SPEAKER_MAX);

    outputchannels = this->mMaxOutputChannels;

    if (this->mSpeakerMode == FMOD_SPEAKERMODE_QUAD || this->mSpeakerMode == FMOD_SPEAKERMODE_PROLOGIC) {
        outputchannels = 6;
    }

    for (int32_t i = 0; i < outputchannels; i++) {
        uint32_t lowest = 361;

        for (int32_t j = 0; j < outputchannels; j++) {
            if (this->mSpeaker[j].mSpeaker == FMOD_SPEAKER_LOW_FREQUENCY) {
                continue;
            }
            if (!this->mSpeaker[j].mActive) {
                continue;
            }
            if (this->mSpeakerMode == FMOD_SPEAKERMODE_QUAD && this->mSpeaker[j].mSpeaker == FMOD_SPEAKER_FRONT_CENTER) {
                continue;
            }

            if (this->mSpeaker[j].mXZAngle < lowest && !used[j]) {
                lowest = this->mSpeaker[j].mXZAngle;
                this->mSpeakerList[i] = &this->mSpeaker[j];
            }
        }
        if (this->mSpeakerList[i]) {
            used[this->mSpeakerList[i]->mSpeaker] = true;
        }
    }
    return FMOD_OK;
}

FMOD_RESULT SystemI::stopDSP(DSPI* dsp) {
    DSPI* channeldsp;

    auto channel = reinterpret_cast<ChannelI*>(this->mChannelUsedListHead.getNext());
    while (channel != &this->mChannelUsedListHead) {
        channel->getCurrentDSP(&channeldsp);
        if (channeldsp == dsp) {
            channel->stop();
        }
        channel = reinterpret_cast<ChannelI*>(channel->getNext());
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::stopSound(SoundI* sound) {
    bool streamstopped = false;

    if (this->mOutput && sound == this->mOutput->mRecordSound) {
        this->recordStop();
    }

    if (sound->isStream()) {
        FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);

        if (!this->mStreamListChannelHead.isEmpty()) {
            auto stream = reinterpret_cast<Stream*>(sound);
            auto current = this->mStreamListChannelHead.getNext();
            while (current != &this->mStreamListChannelHead) {
                auto channelstream = reinterpret_cast<ChannelStream*>(current->getData());
                auto currentstream = reinterpret_cast<Stream*>(channelstream->mSound);
                auto next = current->getNext();

                streamstopped = false;

                if (!currentstream) {
                    current = next;
                    continue;
                }

                if (currentstream == stream) {
                    FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);
                    channelstream->mParent->stop();
                    streamstopped = true;
                    FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);
                } else if (currentstream->mSubSoundList) {
                    if (currentstream->mSubSound[currentstream->mSubSoundList[currentstream->mSubSoundListCurrent]] == stream) {
                        FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);
                        channelstream->mParent->stop();
                        streamstopped = true;
                        FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);
                    }
                } else if (currentstream->mSubSound) {
                    for (int32_t i = 0; i < currentstream->mNumSubSounds; i++) {
                        if (currentstream->mSubSound[i] == sound) {
                            FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);
                            channelstream->mParent->stop();
                            streamstopped = true;
                            FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);
                            break;
                        }
                    }
                } else if (currentstream->mSample == stream->mSample) {
                    FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);
                    channelstream->mParent->stop();
                    streamstopped = true;
                    FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);
                }

                current = next;
            }
        }
    }

    if (sound->mMode & FMOD_SOFTWARE) {
        FMOD_OS_CriticalSection_Enter(this->mDSPLockCrit);
    }

    auto current = reinterpret_cast<ChannelI*>(this->mChannelUsedListHead.getNext());
    while (current != &this->mChannelUsedListHead) {
        auto next = reinterpret_cast<ChannelI*>(current->getNext());

        if (current->mRealChannel[0]) {
            SoundI* channelsound = nullptr;
            current->getCurrentSound(&channelsound);
            if (channelsound == sound) {
                current->stop();
            }
        }

        current = next;
    }

    if (sound->mMode & FMOD_SOFTWARE) {
        FMOD_OS_CriticalSection_Leave(this->mDSPLockCrit);
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::unlockDSP() {
    FMOD_OS_CriticalSection_Leave(this->mDSPLockCrit);
    return FMOD_OK;
}

FMOD_RESULT SystemI::update() {
    FMOD_RESULT result;
    uint32_t currenttimestamp;
    int32_t delta;

    if (this->mInitialized == false) {
        return FMOD_ERR_UNINITIALIZED;
    }

    this->mUpdateTimeStamp.stampIn();

    if (!this->mLastTimeStamp) {
        FMOD_OS_Time_GetMs(&this->mLastTimeStamp);
    }

    FMOD_OS_Time_GetMs(&currenttimestamp);

    if (currenttimestamp >= this->mLastTimeStamp) {
        delta = currenttimestamp - this->mLastTimeStamp;
    } else {
        delta = currenttimestamp;
    }

    this->mLastTimeStamp = currenttimestamp;

    if (this->mEmulated) {
        result = this->mEmulated->update();
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->update3DReverbs();
    if (result != FMOD_OK) {
        return result;
    }

    result = this->updateChannels(delta);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->updateSoundGroups(delta);
    if (result != FMOD_OK) {
        return result;
    }

    if (!this->mSoftware) {
        // XXX
        FMOD_OS_Time_GetMs(&gGlobal->gDSPClockTimeStamp);
        gGlobal->gDSPClock.mHi += delta;
    }

    if (this->mOutput && this->mOutput->mDescription.update) {
        this->mUpdateTimeStamp.setPaused(true);
        this->mOutput->readfrommixer = Output::mixCallback;
        result = this->mOutput->mDescription.update(this->mOutput);
        if (result != FMOD_OK) {
            return result;
        }
        this->mUpdateTimeStamp.setPaused(false);
    }

    result = this->checkDriverList(true);
    if (result != FMOD_OK) {
        return result;
    }

    for (int32_t i = 0; i < this->mNumListeners; i++) {
        this->mListener[i].mMoved = false;
        this->mListener[i].mRotated = false;
    }

    // this->mGeometryMgr.mMoved = false;

    result = AsyncThread::update();
    if (result != FMOD_OK) {
        return result;
    }

    this->mUpdateTimeStamp.stampOut(95);

    if (this->mFlags & FMOD_INIT_STREAM_FROM_UPDATE) {
        this->updateStreams();
    }

    if (this->mFlags & FMOD_INIT_SYNCMIXERWITHUPDATE && this->mOutput->mDescription.polling) {
        reinterpret_cast<OutputPolled*>(this->mOutput)->wakeupThread(false);
    }

    // if (this->mFlags & FMOD_INIT_ENABLE_DSPNET) {
    //     this->mDSPNet.update(this, delta);
    // }

    return FMOD_OK;
}

FMOD_RESULT SystemI::update3DReverbs() {
    FMOD_RESULT result;
    FMOD_REVERB_PROPERTIES ambience;
    FMOD_REVERB_PROPERTIES reverbprops;
    FMOD_REVERB_PROPERTIES qprops;
    FMOD_REVERB_STDPROPERTIES stdprops;
    FMOD_VECTOR pos;
    bool active;
    bool reverb3d_active;
    float direct_occlusion;
    float distance_coeff;
    float distance_gain;
    float reverb_occlusion;
    float t = 0.0f;

    FMOD_memset(&stdprops, 0, sizeof(FMOD_REVERB_STDPROPERTIES));

    auto reverb = reinterpret_cast<ReverbI*>(this->mReverb3DHead.getNext());
    while (reverb != &this->mReverb3DHead) {
        reverb->getActive(&active);
        if (active) {
            reverb->calculateDistanceGain(&this->mListener[0].mPosition, &distance_gain, &distance_coeff);

            if (distance_gain > 0.0f) {
                reverb->get3DAttributes(&pos, nullptr, nullptr);
                // TODO:
                // result = this->mGeometryMgr.lineTestAll(&this->mListener[0].mPosition, &pos, &direct_occlusion, &reverb_occlusion);
                // if (result != FMOD_OK) {
                //     return result;
                // }
                direct_occlusion = 0.0f;
                reverb_occlusion = 0.0f;

                distance_gain *= (1.0f - reverb_occlusion);
                distance_coeff *= (1.0f - reverb_occlusion);
            }

            if (reverb->getMode() == FMOD_REVERB_PHYSICAL) {
                if (reverb->getGain() != distance_gain) {
                    reverb->setGain(distance_gain);
                }
            } else if (reverb->getMode() == FMOD_REVERB_VIRTUAL) {
                reverb->getProperties(&reverbprops);
                ReverbI::sumProps(&stdprops, &reverbprops, distance_coeff);
                t += distance_coeff;
            }
        }

        reverb = reinterpret_cast<ReverbI*>(reverb->getNext());
    }

    this->get3DReverbActive(&reverb3d_active);
    if (!reverb3d_active) {
        return FMOD_OK;
    }

    if (t < 1.0f) {
        this->getReverbAmbientProperties(&ambience);
        if (ambience.Environment == -1) {
            ambience.Room = -10000;
            ReverbI::sumRoomProps(&stdprops, &ambience, 1.0f - t);
        } else {
            ReverbI::sumProps(&stdprops, &ambience, 1.0f - t);
        }

        t = 1.0f;
    }

    ReverbI::factorProps(&qprops, &stdprops, 1.0f / t);
    this->set3DReverbProperties(&qprops, true);
    return FMOD_OK;
}

FMOD_RESULT SystemI::updateChannels(int32_t delta) {
    ChannelI* emuchannel;
    FMOD_MODE channelmode;
    FMOD_RESULT result = FMOD_OK;
    LinkedListNode* realcurrent[CHANNELREAL_TYPE_MAX];
    bool isvirtual;
    int32_t emulatedused;
    int32_t realchannelsneeded;
    int32_t realchannelsfound;
    int32_t channelsfound;

    auto current = reinterpret_cast<ChannelI*>(this->mChannelUsedListHead.getNext());
    while (current != &this->mChannelUsedListHead) {
        auto next = reinterpret_cast<ChannelI*>(current->getNext());
        bool playing = false;

        current->isPlaying(&playing);
        if (playing) {
            current->update(delta);
            current->getMode(&channelmode);

            if (!(channelmode & FMOD_VIRTUAL_PLAYFROMSTART)) {
                current->mFlags &= ~CHANNELI_FLAG_JUSTWENTVIRTUAL;
            }
        }

        current->isPlaying(&playing);
        if (!playing) {
            current->stopEx(true, true, true, true, true, true, false);
        }

        current = next;
    }

    if (this->mEmulated) {
        this->mEmulated->mChannelPool->getChannelsUsed(&emulatedused);
        if (emulatedused) {
            auto emucurrent = this->mChannelSortedListHead.getNext();

            for (int32_t i = 0; i < CHANNELREAL_TYPE_MAX; i++) {
                realcurrent[i] = &this->mChannelSortedListHead;
            }

            do {
                ChannelReal* emuchannelreal[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { nullptr };
                ChannelI* realchannel[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { nullptr };
                ChannelReal* realchannelreal[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { nullptr };
                FMOD_MODE mode;

                do {
                    emuchannel = reinterpret_cast<ChannelI*>(emucurrent->getData());
                    if (!emuchannel) {
                        break;
                    }

                    result = emuchannel->isVirtual(&isvirtual);
                    if (isvirtual && !(emuchannel->mFlags & CHANNELI_FLAG_FORCEVIRTUAL)) {
                        break;
                    }

                    emucurrent = emucurrent->getNext();
                } while (emucurrent != &this->mChannelSortedListHead);

                if (emucurrent == &this->mChannelSortedListHead ||
                    emucurrent == realcurrent[0] ||
                    emucurrent == realcurrent[1] ||
                    emucurrent == realcurrent[2] ||
                    result != FMOD_OK) {
                    break;
                }

                channelsfound = 0;
                realchannelsfound = 0;

                result = emuchannel->getRealChannel(emuchannelreal, nullptr);
                if (result != FMOD_OK) {
                    break;
                }

                mode = emuchannelreal[0]->mMode;

                if (emuchannelreal[0]->mSound) {
                    if (emuchannelreal[0]->mSound->isStream()) {
                        auto stream = reinterpret_cast<Stream*>(emuchannelreal[0]->mSound);
                        auto sample = stream->mSample;
                        realchannelsneeded = sample->mNumSubSamples;
                    } else {
                        auto sample = reinterpret_cast<Sample*>(emuchannelreal[0]->mSound);
                        realchannelsneeded = sample->mNumSubSamples;
                    }
                } else if (emuchannelreal[0]->mDSP) {
                    realchannelsneeded = 1;
                } else {
                    return FMOD_ERR_INTERNAL;
                }

                if (realchannelsneeded < 1) {
                    realchannelsneeded = 1;
                }

                if (realchannelsfound < realchannelsneeded) {
                    int32_t found = 0;
                    ChannelReal* tempchannelreal[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { nullptr };

                    if (mode & FMOD_HARDWARE) {
                        if (emuchannelreal[0]->mSound) {
                            result = this->mOutput->getFreeChannel(mode, tempchannelreal, realchannelsneeded, emuchannelreal[0]->mSound->mChannels, &found);
                        } else {
                            return FMOD_ERR_INTERNAL;
                        }
                    } else {
                        if (mode & FMOD_CREATECOMPRESSEDSAMPLE) {
                            auto sample = reinterpret_cast<Sample*>(emuchannelreal[0]->mSound);
                            if (sample->mFormat == FMOD_SOUND_FORMAT_MPEG) {
                                result = this->mDSPCodecPool_MPEG.areAnyFree();
                            } else if (sample->mFormat == FMOD_SOUND_FORMAT_IMAADPCM) {
                                result = this->mDSPCodecPool_ADPCM.areAnyFree();
                            }
                        }

                        if (result == FMOD_OK) {
                            if (emuchannelreal[0]->mSound) {
                                result = this->mSoftware->getFreeChannel(mode, tempchannelreal, realchannelsneeded, emuchannelreal[0]->mSound->mChannels, &found);
                            } else if (emuchannelreal[0]->mDSP) {
                                result = this->mSoftware->getFreeChannel(mode, tempchannelreal, realchannelsneeded, 1, &found);
                            } else {
                                return FMOD_ERR_INTERNAL;
                            }
                        }
                    }

                    for (int32_t i = 0; i < found; i++) {
                        realchannelreal[realchannelsfound] = tempchannelreal[i];
                        realchannelsfound++;
                    }
                }

tryformore:
                if (realchannelsfound < realchannelsneeded) {
                    CHANNELREAL_TYPE realtype;

                    if (this->mOutput->mChannelPool == this->mOutput->mChannelPool3D) {
                        realtype = mode & FMOD_SOFTWARE ? CHANNELREAL_TYPE_SW : CHANNELREAL_TYPE_HW3D;
                    } else if (mode & FMOD_SOFTWARE) {
                        realtype = CHANNELREAL_TYPE_SW;
                    } else {
                        realtype = mode & FMOD_3D ? CHANNELREAL_TYPE_HW3D : CHANNELREAL_TYPE_HW2D;
                    }

                    if (realcurrent[realtype] == &this->mChannelSortedListHead) {
                        realcurrent[realtype] = realcurrent[realtype]->getPrev();
                    }

                    do {
                        if (realcurrent[realtype] == emucurrent) {
                            break;
                        }

                        realchannel[channelsfound] = reinterpret_cast<ChannelI*>(realcurrent[realtype]->getData());
                        if (realcurrent[realtype] == &this->mChannelSortedListHead) {
                            break;
                        }

                        realchannel[channelsfound]->isVirtual(&isvirtual);
                        if (!isvirtual) {
                            FMOD_MODE currentmode;
                            CHANNELREAL_TYPE currentrealtype;

                            realchannel[channelsfound]->getMode(&currentmode);

                            if (this->mOutput->mChannelPool == this->mOutput->mChannelPool3D) {
                                currentrealtype = currentmode & FMOD_SOFTWARE ? CHANNELREAL_TYPE_SW : CHANNELREAL_TYPE_HW3D;
                            } else if (currentmode & FMOD_SOFTWARE) {
                                currentrealtype = CHANNELREAL_TYPE_SW;
                            } else {
                                currentrealtype = currentmode & FMOD_3D ? CHANNELREAL_TYPE_HW3D : CHANNELREAL_TYPE_HW2D;
                            }

                            if (realtype == currentrealtype) {
                                if (realtype == CHANNELREAL_TYPE_SW) {
                                    if (mode & FMOD_CREATECOMPRESSEDSAMPLE) {
                                        auto sample = reinterpret_cast<Sample*>(emuchannelreal[0]->mSound);
                                        if (sample->mFormat == FMOD_SOUND_FORMAT_MPEG) {
                                            result = this->mDSPCodecPool_MPEG.areAnyFree();
                                        } else if (sample->mFormat == FMOD_SOUND_FORMAT_IMAADPCM) {
                                            result = this->mDSPCodecPool_ADPCM.areAnyFree();
                                        }
                                        if (result == FMOD_OK) {
                                            break;
                                        }

                                        if (currentmode & FMOD_CREATECOMPRESSEDSAMPLE) {
                                            ChannelReal* tempcr[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { nullptr };
                                            realchannel[channelsfound]->getRealChannel(tempcr, nullptr);

                                            auto currentsample = reinterpret_cast<Sample*>(tempcr[0]->mSound);

                                            if (currentsample->mFormat == sample->mFormat) {
                                                result = FMOD_OK;
                                                break;
                                            }
                                        }

                                    } else {
                                        result = FMOD_OK;
                                        break;
                                    }
                                } else {
                                    result = FMOD_OK;
                                    break;
                                }
                            }
                        }

                        realcurrent[realtype] = realcurrent[realtype]->getPrev();
                    } while (1);

                    if (result == FMOD_OK) {
                        int32_t found = 0;
                        ChannelReal* tempchannelreal[FMOD_CHANNEL_MAXREALSUBCHANNELS] = { nullptr };
                        realchannel[channelsfound]->getRealChannel(tempchannelreal, &found);
                        for (int32_t i = 0; i < found; i++) {
                            realchannelreal[realchannelsfound] = tempchannelreal[i];
                            realchannelsfound++;
                        }
                        channelsfound++;
                        if (realchannelsfound < realchannelsneeded) {
                            realcurrent[realtype] = realcurrent[realtype]->getPrev();
                            goto tryformore;
                        }
                    } else {
                        realchannel[channelsfound] = nullptr;
                    }
                }

                if (realchannelsfound >= realchannelsneeded) {
                    FMOD_CHANNEL_INFO emuinfo;
                    FMOD_CHANNEL_INFO realinfo[FMOD_CHANNEL_MAXREALSUBCHANNELS];
                    FMOD_MODE mode;
                    auto emulatedtoswap = channelsfound;

                    emuchannel->getMode(&mode);

                    if (channelsfound) {
                        for (int32_t i = 0; i < channelsfound; i++) {
                            realchannel[i]->getChannelInfo(&realinfo[i]);
                            realchannel[i]->stopEx(false, false, false, true, false, false, false);
                        }

                        for (int32_t i = realchannelsneeded; i < realchannelsfound; i++) {
                            realchannelreal[i]->mFlags &= ~(CHANNELREAL_FLAG_IN_USE | CHANNELREAL_FLAG_ALLOCATED | CHANNELREAL_FLAG_PLAYING | CHANNELREAL_FLAG_PAUSED);
                            realchannelreal[i]->mFlags |= CHANNELREAL_FLAG_STOPPED;
                        }
                    } else {
                        emuchannel->getChannelInfo(&emuinfo);
                        emuchannel->stopEx(false, false, false, true, false, false, true);

                        if (emuinfo.mSound && emuinfo.mSound->mMode & FMOD_CREATESTREAM) {
                            auto stream = reinterpret_cast<Stream*>(emuinfo.mSound);
                            auto channelstream = stream->mChannel;

                            emuchannel->mRealChannel[0] = channelstream;

                            channelstream->mNumSubChannels = realchannelsneeded;
                            for (int32_t i = 0; i < realchannelsneeded; i++) {
                                channelstream->mSubChannel[i] = realchannelreal[i];
                                channelstream->mSubChannel[i]->mSubChannelIndex = i;
                            }

                        } else {
                            emuchannel->mNumRealChannels = realchannelsneeded;
                            for (int32_t i = 0; i < realchannelsneeded; i++) {
                                emuchannel->mRealChannel[i] = realchannelreal[i];
                                emuchannel->mRealChannel[i]->mSubChannelIndex = i;
                            }
                        }

                        if (emuinfo.mDSP) {
                            result = emuchannel->play(emuinfo.mDSP, true, false, false);
                        } else {
                            result = emuchannel->play(emuinfo.mSound, true, false, false);
                        }

                        emuchannel->setChannelInfo(&emuinfo);
                        emuchannel->setChannelGroup(emuchannel->mChannelGroup);
                        emuchannel->setPaused(emuinfo.mPaused);
                    }

                    for (int32_t i = 0; i < emulatedtoswap; i++) {
                        result = this->mEmulated->getFreeChannel(mode, emuchannelreal, 1, 1, nullptr);

                        realchannel[i]->mNumRealChannels = 1;
                        realchannel[i]->mRealChannel[0] = emuchannelreal[0];

                        if (realinfo[i].mSound) {
                            result = realchannel[i]->play(realinfo[i].mSound->mSubSampleParent, true, false, false);
                            realchannel[i]->setChannelInfo(&realinfo[i]);
                            realchannel[i]->setChannelGroup(realchannel[i]->mChannelGroup);
                            realchannel[i]->setPaused(realinfo[i].mPaused);
                        } else if (realinfo[i].mDSP) {
                            result = realchannel[i]->play(realinfo[i].mDSP, true, false, false);
                            realchannel[i]->setChannelInfo(&realinfo[i]);
                            realchannel[i]->setChannelGroup(realchannel[i]->mChannelGroup);
                            realchannel[i]->setPaused(realinfo[i].mPaused);
                        }

                        emucurrent = reinterpret_cast<ChannelI*>(emucurrent->getNext());
                    }
                }
            } while (1);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::updateSoundGroups(int32_t delta) {
    auto soundgroup = static_cast<SoundGroupI*>(this->mSoundGroupUsedHead.getNext());
    bool mute = false;

    while (soundgroup != &this->mSoundGroupUsedHead) {
        auto next = static_cast<SoundGroupI*>(soundgroup->getNext());

        soundgroup->mPlayCount = 0;
        soundgroup->getNumPlaying(&soundgroup->mNumAudible);
        if (!soundgroup->mNumAudible) {
            soundgroup->removeNode();
            soundgroup->addAfter(&this->mSoundGroupFreeHead);
        } else if (soundgroup->mMaxAudibleBehavior == FMOD_SOUNDGROUP_BEHAVIOR_MUTE) {
            mute = true;
        }

        soundgroup = next;
    }

    if (mute) {
        auto current = this->mChannelSortedListHead.getNext();
        while (current != &this->mChannelSortedListHead) {
            auto channel = reinterpret_cast<ChannelI*>(current->getData());
            auto next = current->getNext();

            if (channel->mRealChannel[0]) {
                auto sound = channel->mRealChannel[0]->mSound;
                if (sound && sound->mSoundGroup) {
                    auto soundgroup = sound->mSoundGroup;

                    if (soundgroup->mMaxAudibleBehavior == FMOD_SOUNDGROUP_BEHAVIOR_MUTE && soundgroup->mMaxAudible >= 0) {
                        soundgroup->mPlayCount++;

                        if (soundgroup->mPlayCount > soundgroup->mMaxAudible) {
                            channel->mFadeTarget = 0.0f;
                        } else {
                            channel->mFadeTarget = 1.0f;
                        }
                    }

                    if (channel->mFadeVolume != channel->mFadeTarget) {
                        if (soundgroup->mFadeSpeed < 0.001f) {
                            channel->mFadeVolume = channel->mFadeTarget;
                        } else {
                            if (channel->mFadeVolume < channel->mFadeTarget) {
                                channel->mFadeVolume += (static_cast<float>(delta) / (soundgroup->mFadeSpeed * 1000.0f));
                                if (channel->mFadeVolume > channel->mFadeTarget) {
                                    channel->mFadeVolume = channel->mFadeTarget;
                                }
                            }
                            if (channel->mFadeVolume > channel->mFadeTarget) {
                                channel->mFadeVolume -= (static_cast<float>(delta) / (soundgroup->mFadeSpeed * 1000.0f));
                                if (channel->mFadeVolume < channel->mFadeTarget) {
                                    channel->mFadeVolume = channel->mFadeTarget;
                                }
                            }
                        }

                        channel->setVolume(channel->mVolume, false);
                    }
                }
            }

            current = next;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SystemI::updateStreams() {
    this->mStreamTimeStamp.stampIn();

    FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);

    this->mStreamListChannelCurrent = this->mStreamListChannelHead.getNext();

    while (this->mStreamListChannelCurrent != &this->mStreamListChannelHead) {
        auto channelstream = reinterpret_cast<ChannelStream*>(this->mStreamListChannelCurrent->getData());
        this->mStreamListChannelNext = this->mStreamListChannelCurrent->getNext();
        FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);

        FMOD_OS_CriticalSection_Enter(this->mStreamUpdateCrit);
        if (!channelstream->mFinished) {
            channelstream->updateStream();
        }
        FMOD_OS_CriticalSection_Leave(this->mStreamUpdateCrit);

        FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);
        this->mStreamListChannelCurrent = this->mStreamListChannelNext;
    }

    FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);
    FMOD_OS_CriticalSection_Enter(this->mStreamListCrit);

    auto current = this->mStreamListSoundHead.getNext();

    while (current != &this->mStreamListSoundHead) {
        auto stream = reinterpret_cast<Stream*>(current->getData());
        if (stream->mChannel && stream->mChannel->mFinished) {
            stream->mFlags |= FMOD_SOUND_FLAG_FINISHED;
        }
        current = current->getNext();
    }

    FMOD_OS_CriticalSection_Leave(this->mStreamListCrit);
    this->mStreamTimeStamp.stampOut(95);

    return FMOD_OK;
}

uint32_t SystemI::count3DPhysicalReverbs() {
    uint32_t count = 0;

    auto reverb = reinterpret_cast<ReverbI*>(this->mReverb3DHead.getNext());

    while (reverb != &this->mReverb3DHead) {
        if (reverb->getMode() == FMOD_REVERB_PHYSICAL) {
            count++;
        }

        reverb = reinterpret_cast<ReverbI*>(reverb->getNext());
    }

    return count;
}

uint32_t SystemI::count3DVirtualReverbs() {
    uint32_t count = 0;

    auto reverb = reinterpret_cast<ReverbI*>(this->mReverb3DHead.getNext());

    while (reverb != &this->mReverb3DHead) {
        if (reverb->getMode() == FMOD_REVERB_VIRTUAL) {
            count++;
        }

        reverb = reinterpret_cast<ReverbI*>(reverb->getNext());
    }

    return count;
}

FMOD_RESULT SystemI::getInstance(uint32_t id, SystemI** sys) {
    if (sys) {
        *sys = nullptr;
    }

    auto current = reinterpret_cast<SystemI*>(gGlobal->gSystemHead->getNext());
    while (current != gGlobal->gSystemHead) {
        if (current->mIndex == id) {
            if (sys) {
                *sys = current;
            }
            return FMOD_OK;
        }

        current = reinterpret_cast<SystemI*>(current->getNext());
    }

    return FMOD_ERR_INVALID_PARAM;
}

FMOD_RESULT SystemI::validate(System* system, SystemI** systemi) {
    if (!system) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (!systemi) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto sys = reinterpret_cast<SystemI*>(system);
    if (!gGlobal->gSystemHead->exists(sys)) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *systemi = sys;

    return FMOD_OK;
}

void SystemI::streamThread(void* data) {
    reinterpret_cast<SystemI*>(data)->updateStreams();
}

} // namespace FMOD

FMOD_RESULT F_API FMOD_System_Create(FMOD_SYSTEM** system) {
    FMOD::SystemI* sys;
    bool usedsys[16];
    int32_t count;

    if (!system) {
        return FMOD_ERR_INVALID_PARAM;
    }

    sys = FMOD_Object_Calloc(FMOD::SystemI);

    *system = reinterpret_cast<FMOD_SYSTEM*>(sys);
    if (!*system) {
        return FMOD_ERR_MEMORY;
    }

    FMOD_memset(usedsys, 0, sizeof(usedsys));
    auto current = reinterpret_cast<FMOD::SystemI*>(FMOD::gGlobal->gSystemHead->getNext());
    while (current != FMOD::gGlobal->gSystemHead) {
        usedsys[current->mIndex - 1] = true;
    }

    for (count = 0; count < 15; count++) {
        if (!usedsys[count]) {
            sys->mIndex = count + 1;
            break;
        }
    }

    if (count == 15) {
        FMOD_Memory_Free(sys);
        return FMOD_ERR_MEMORY;
    }

    sys->addAfter(FMOD::gGlobal->gSystemHead);

    return FMOD_OK;
}

FMOD_RESULT F_API FMOD_System_Release(FMOD_SYSTEM* system) {
    auto sys = reinterpret_cast<FMOD::System*>(system);
    return sys->release();
}
