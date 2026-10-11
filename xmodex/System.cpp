#include "fmod.hpp"
#include "xmodex/SystemI.hpp"

namespace FMOD {

FMOD_RESULT F_API System::release() {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->release();
}

FMOD_RESULT F_API System::setOutput(FMOD_OUTPUTTYPE output) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setOutput(output);
}

FMOD_RESULT F_API System::getOutput(FMOD_OUTPUTTYPE* output) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getOutput(output);
}

FMOD_RESULT F_API System::getNumDrivers(int32_t* numdrivers) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getNumDrivers(numdrivers);
}

FMOD_RESULT F_API System::getDriver(int32_t* driver) {
    FMOD_RESULT result;
    SystemI* systemi;

    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return systemi->getDriver(driver);
    }
}

FMOD_RESULT F_API System::getDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getDriverName(id, name, namelen);
}

FMOD_RESULT F_API System::getDriverCaps(int32_t id, FMOD_CAPS* caps, int32_t* minfrequency, int32_t* maxfrequency, FMOD_SPEAKERMODE* controlpanelspeakermode) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getDriverCaps(id, caps, minfrequency, maxfrequency, controlpanelspeakermode);
}

FMOD_RESULT F_API System::setDriver(int32_t driver) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setDriver(driver);
}

FMOD_RESULT F_API System::setHardwareChannels(int32_t min2d, int32_t max2d, int32_t min3d, int32_t max3d) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setHardwareChannels(min2d, max2d, min3d, max3d);
}

FMOD_RESULT F_API System::setSoftwareChannels(int32_t numsoftwarechannels) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setSoftwareChannels(numsoftwarechannels);
}

FMOD_RESULT F_API System::getSoftwareChannels(int32_t* numsoftwarechannels) {
    FMOD_RESULT result;
    SystemI* systemi;

    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return systemi->getSoftwareChannels(numsoftwarechannels);
    }
}

FMOD_RESULT F_API System::setSoftwareFormat(int32_t samplerate, FMOD_SOUND_FORMAT format, int32_t numoutputchannels, int32_t maxinputchannels, FMOD_DSP_RESAMPLER resamplemethod) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setSoftwareFormat(samplerate, format, numoutputchannels, maxinputchannels, resamplemethod);
}

FMOD_RESULT F_API System::getSoftwareFormat(int32_t* samplerate, FMOD_SOUND_FORMAT* format, int32_t* numoutputchannels, int32_t* maxinputchannels, FMOD_DSP_RESAMPLER* resamplemethod, int32_t* bits) {
    FMOD_RESULT result;
    SystemI* systemi;

    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return systemi->getSoftwareFormat(samplerate, format, numoutputchannels, maxinputchannels, resamplemethod, bits);
    }
}

FMOD_RESULT F_API System::setDSPBufferSize(uint32_t bufferlength, int32_t numbuffers) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setDSPBufferSize(bufferlength, numbuffers);
}

FMOD_RESULT F_API System::setSpeakerMode(FMOD_SPEAKERMODE speakermode) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setSpeakerMode(speakermode);
}

FMOD_RESULT F_API System::setCallback(FMOD_SYSTEM_CALLBACKTYPE type, FMOD_SYSTEM_CALLBACK callback) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setCallback(type, callback);
}

FMOD_RESULT F_API System::init(int32_t maxchannels, FMOD_INITFLAGS flags, void* extradriverdata) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->init(maxchannels, flags, extradriverdata);
}

FMOD_RESULT F_API System::close() {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->close();
}

FMOD_RESULT F_API System::update() {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->update();
}

FMOD_RESULT F_API System::set3DSettings(float dopplerscale, float distancefactor, float rolloffscale) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->set3DSettings(dopplerscale, distancefactor, rolloffscale);
}

FMOD_RESULT F_API System::set3DListenerAttributes(int32_t listener, const FMOD_VECTOR* pos, const FMOD_VECTOR* vel, const FMOD_VECTOR* forward, const FMOD_VECTOR* up) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->set3DListenerAttributes(listener, pos, vel, forward, up);
}

FMOD_RESULT F_API System::get3DListenerAttributes(int32_t listener, FMOD_VECTOR* pos, FMOD_VECTOR* vel, FMOD_VECTOR* forward, FMOD_VECTOR* up) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->get3DListenerAttributes(listener, pos, vel, forward, up);
}

FMOD_RESULT F_API System::set3DRolloffCallback(FMOD_3D_ROLLOFFCALLBACK callback) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->set3DRolloffCallback(callback);
}

FMOD_RESULT F_API System::getVersion(uint32_t* version) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getVersion(version);
}

FMOD_RESULT F_API System::getHardwareChannels(int32_t* num2d, int32_t* num3d, int32_t* total) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getHardwareChannels(num2d, num3d, total);
}

FMOD_RESULT F_API System::createSound(const char* name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO* exinfo, Sound** sound) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->createSound(name_or_data, mode, exinfo, reinterpret_cast<SoundI**>(sound));
}

FMOD_RESULT F_API System::createStream(const char* name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO* exinfo, Sound** sound) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->createStream(name_or_data, mode, exinfo, reinterpret_cast<SoundI**>(sound));
}

FMOD_RESULT F_API System::createDSP(FMOD_DSP_DESCRIPTION* description, DSP** dsp) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->createDSP(description, reinterpret_cast<DSPI**>(dsp));
}

FMOD_RESULT F_API System::createDSPByType(FMOD_DSP_TYPE type, DSP** dsp) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->createDSPByType(type, reinterpret_cast<DSPI**>(dsp));
}

FMOD_RESULT F_API System::createChannelGroup(const char* name, ChannelGroup** channelgroup) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->createChannelGroup(name, reinterpret_cast<ChannelGroupI**>(channelgroup));
}

FMOD_RESULT F_API System::createSoundGroup(const char* name, SoundGroup** soundgroup) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->createSoundGroup(name, reinterpret_cast<SoundGroupI**>(soundgroup));
}

FMOD_RESULT F_API System::playSound(FMOD_CHANNELINDEX channelid, Sound* sound, bool paused, Channel** channel) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->playSound(channelid, reinterpret_cast<SoundI*>(sound), paused, reinterpret_cast<ChannelI**>(channel));
}

FMOD_RESULT F_API System::playDSP(FMOD_CHANNELINDEX channelid, DSP* dsp, bool paused, Channel** channel) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->playDSP(channelid, reinterpret_cast<DSPI*>(dsp), paused, reinterpret_cast<ChannelI**>(channel));
}

FMOD_RESULT F_API System::getChannel(int32_t channelid, Channel** channel) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getChannel(channelid, reinterpret_cast<ChannelI**>(channel));
}

FMOD_RESULT F_API System::getMasterChannelGroup(ChannelGroup** channelgroup) {
    FMOD_RESULT result;
    SystemI* systemi;

    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    } else {
        return systemi->getMasterChannelGroup(reinterpret_cast<ChannelGroupI**>(channelgroup));
    }
}

FMOD_RESULT F_API System::setReverbProperties(FMOD_REVERB_PROPERTIES* prop) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setReverbProperties(prop, false);
}

FMOD_RESULT F_API System::setRecordDriver(int32_t recorddriver) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->setRecordDriver(recorddriver);
}

FMOD_RESULT F_API System::getRecordNumDrivers(int32_t* numdrivers) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getRecordNumDrivers(numdrivers);
}

FMOD_RESULT F_API System::getRecordDriverName(int32_t id, char* name, int32_t namelen) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getRecordDriverName(id, name, namelen);
}

FMOD_RESULT F_API System::getRecordPosition(uint32_t* position) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->getRecordPosition(position);
}

FMOD_RESULT F_API System::recordStart(Sound* sound, bool loop) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->recordStart(reinterpret_cast<SoundI*>(sound), loop);
}

FMOD_RESULT F_API System::recordStop() {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->recordStop();
}

FMOD_RESULT F_API System::isRecording(bool* recording) {
    FMOD_RESULT result;
    SystemI* systemi;
    result = SystemI::validate(this, &systemi);
    if (result != FMOD_OK) {
        return result;
    }
    return systemi->isRecording(recording);
}

} // namespace FMOD
