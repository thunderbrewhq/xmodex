#include "xmodex/sound/SoundI.hpp"
#include "fmod.hpp"
#include "xmodex/AsyncData.hpp"
#include "xmodex/AsyncThread.hpp"
#include "xmodex/LocalCriticalSection.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SyncPoint.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/channel/ChannelI.hpp"
#include "xmodex/channel/ChannelStream.hpp"
#include "xmodex/codec/Codec.hpp"
#include "xmodex/file/File.hpp"
#include "xmodex/os/Thread.hpp"
#include "xmodex/os/Time.hpp"
#include "xmodex/sound/Sample.hpp"
#include "xmodex/sound/SoundGroupI.hpp"
#include "xmodex/sound/Stream.hpp"

namespace FMOD {

SoundI::SoundI() {
    this->mMinDistance = 1.0f;
    this->mMaxDistance = 10000.0f;
    this->mMode = 0;
    this->mConeInsideAngle = 360.0f;
    this->mFormat = FMOD_SOUND_FORMAT_NONE;
    this->mConeOutsideAngle = 360.0f;
    this->mName = nullptr;
    this->mChannels = 1;
    this->mConeOutsideVolume = 1.0f;
    this->mDefaultPriority = FMOD_CHANNEL_DEFAULTPRIORITY;
    this->mDefaultVolume = 1.0f;
    this->mLoopStart = 0;
    this->mLoopLength = 0;
    this->mDefaultFrequency = 44100.0f;
    this->mLoopCount = -1;
    this->mUserData = nullptr;
    this->mDefaultPan = 0.0f;
    this->mMemoryUsed = 0;
    this->mFrequencyVariation = 0.0f;
    this->mCodec = nullptr;
    this->mVolumeVariation = 0.0f;
    this->mSubSound = nullptr;
    this->mPanVariation = 0.0f;
    this->mSubSoundParent = nullptr;
    this->mSubSoundList = nullptr;
    this->mNumSubSounds = 0;
    this->mNumActiveSubSounds = 0;
    this->mAsyncData = nullptr;
    this->mNumSyncPoints = 0;
    this->mOpenState = FMOD_OPENSTATE_READY;
    this->mSubSampleParent = this;
    this->mRolloffPoint = nullptr;
    this->mNumRolloffPoints = 0;
    this->mSyncPointHead = nullptr;
    this->mPostReadCallback = nullptr;
    this->mPostSetPositionCallback = nullptr;
    this->mPostCallbackSound = nullptr;
    this->mFlags = 0;
    this->mSoundGroupNode.initNode();
}

bool SoundI::readyForRelease() {
    if (this->mCodec) {
        if (!this->mCodec->readyForRelease()) {
            return false;
        }
    }

    if (this->isStream()) {
        return this->mFlags & FMOD_SOUND_FLAG_FINISHED;
    }

    return true;
}

FMOD_RESULT SoundI::seek(int32_t subsound, uint32_t position) {
    FMOD_RESULT result;

    if (!this->mCodec->mFile) {
        return FMOD_ERR_FILE_COULDNOTSEEK;
    }

    result = this->mCodec->setPosition(subsound, position, FMOD_TIMEUNIT_PCM);

    this->mCodec->getPosition(&position, FMOD_TIMEUNIT_PCM);

    this->mPosition = position;

    if (this->mPostSetPositionCallback) {
        this->mPostSetPositionCallback(
            reinterpret_cast<FMOD_SOUND*>(this),
            subsound,
            position,
            FMOD_TIMEUNIT_PCM);
    }

    return result;
}

FMOD_RESULT SoundI::updateSubSound(int32_t index, bool updateinfoonly) {
    FMOD_RESULT result;
    FMOD_CODEC_WAVEFORMAT waveformat;
    FMOD_UINT_NATIVE threadid;
    LocalCriticalSection crit(this->mSystem->mStreamUpdateCrit);
    bool frommainthread = false;

    FMOD_OS_Thread_GetCurrentID(&threadid);

    if (threadid == this->mSystem->mMainThreadID && !updateinfoonly) {
        this->mSubSoundIndex = index;
        this->mSystem->stopSound(this);

        if (this->mMode & FMOD_NONBLOCKING && this->isStream()) {
            return FMOD_OK;
        }
    }

    if (this->isStream() && this->mFlags & FMOD_SOUND_FLAG_PLAYED) {
        auto stream = static_cast<Stream*>(this);

        if (!(stream->mFlags & FMOD_SOUND_FLAG_FINISHED)) {
            crit.enter();
        }

        stream->mChannel->mFinished = false;
    }

    this->mFlags &= ~FMOD_SOUND_FLAG_FINISHED;
    this->mSubSoundIndex = index;

    result = this->mCodec->mDescription.getwaveformat(
        this->mCodec,
        index,
        &waveformat);
    if (result != FMOD_OK) {
        return result;
    }

    this->mChannels = waveformat.channels;
    this->mDefaultFrequency = waveformat.frequency;
    this->mFormat = waveformat.format;
    this->mDefaultChannelMask = waveformat.channelmask;
    this->mLength = waveformat.lengthpcm;
    this->mLoopStart = waveformat.loopstart;
    this->mLoopLength = (waveformat.loopend - waveformat.loopstart) + 1;

    this->setLoopPoints(
        waveformat.loopstart, FMOD_TIMEUNIT_PCM,
        waveformat.loopstart, FMOD_TIMEUNIT_PCM);

    return FMOD_OK;
}

FMOD_RESULT SoundI::read(uint32_t offset, uint32_t numsamples, uint32_t* read) {
    FMOD_RESULT result;
    uint32_t offbytes;
    uint32_t numbytes;
    uint32_t readbytes;
    uint32_t chunksize;
    FMOD_CODEC_WAVEFORMAT waveformat;

    this->mPosition = offset;

    if (this->mMode & FMOD_CREATECOMPRESSEDSAMPLE) {
        offbytes = offset;
        numbytes = numsamples;
    } else {
        this->getBytesFromSamples(offset, &offbytes, false);
        this->getBytesFromSamples(numsamples, &numbytes, false);
    }

    readbytes = 0;

    if (read) {
        *read = 0;
    }

    result = this->mCodec->mDescription.getwaveformat(this->mCodec, this->mCodec->mSubSoundIndex, &waveformat);
    if (result != FMOD_OK) {
        return result;
    }

    chunksize = SOUND_READCHUNKSIZE;

    if (this->mMode & FMOD_CREATESTREAM) {
        if (this->mCodec->mBlockAlign) {
            chunksize /= this->mCodec->mBlockAlign;
            chunksize *= this->mCodec->mBlockAlign;

            if (!chunksize) {
                chunksize = this->mCodec->mBlockAlign;
            }
        }
    } else {
        if (waveformat.blockalign) {
            chunksize /= waveformat.blockalign;
            chunksize *= waveformat.blockalign;

            if (!chunksize) {
                chunksize = waveformat.blockalign;
            }
        }
    }

    while (numbytes) {
        int32_t size = numbytes > chunksize ? chunksize : numbytes;
        uint32_t bytesreadtotal = 0;
        void* ptr1;
        void* ptr2;
        uint32_t len1;
        uint32_t len2;

        result = this->lock(offbytes, size, &ptr1, &ptr2, &len1, &len2);
        if (result != FMOD_OK) {
            return result;
        }

        if (!len1 && !len2) {
            this->unlock(ptr1, ptr2, len1, len2);
            return FMOD_ERR_FILE_BAD;
        }

        if (ptr1 && len1) {
            uint32_t bytesread = 0;

            result = this->readData(ptr1, len1, &bytesread);
            if ((result != FMOD_OK) && (result != FMOD_ERR_FILE_EOF)) {
                this->unlock(ptr1, ptr2, len1, len2);
                return result;
            }

            bytesreadtotal += bytesread;
        }

        if (ptr2 && len2) {
            uint32_t bytesread = 0;

            result = this->readData(ptr2, len2, &bytesread);
            if ((result != FMOD_OK) && (result != FMOD_ERR_FILE_EOF)) {
                this->unlock(ptr1, ptr2, len1, len2);
                return result;
            }

            bytesreadtotal += bytesread;
        }

        if (result == FMOD_ERR_FILE_EOF) {
            numbytes = bytesreadtotal;
        }

        FMOD_RESULT result2 = this->unlock(ptr1, ptr2, len1, len2);
        if (result2 != FMOD_OK) {
            return result2;
        }

        numbytes -= bytesreadtotal;
        offbytes += bytesreadtotal;
        readbytes += bytesreadtotal;
    }

    if (read) {
        this->getSamplesFromBytes(readbytes, read);
    }

    return result;
}

FMOD_RESULT SoundI::clear(uint32_t offset, uint32_t numsamples) {
    FMOD_RESULT result;
    uint32_t offbytes;
    uint32_t numbytes;
    void* ptr1;
    void* ptr2;
    uint32_t len1;
    uint32_t len2;
    uint32_t chunksize;
    FMOD_CODEC_WAVEFORMAT waveformat;

    if (!numsamples) {
        return FMOD_OK;
    }

    this->getBytesFromSamples(offset, &offbytes, false);
    this->getBytesFromSamples(numsamples, &numbytes, false);

    chunksize = SOUND_READCHUNKSIZE;

    while (numbytes) {
        int32_t size = numbytes > chunksize ? chunksize : numbytes;
        int32_t bytesreadtotal = 0;

        result = this->lock(offbytes, size, &ptr1, &ptr2, &len1, &len2);
        if (result != FMOD_OK) {
            return result;
        }

        if (ptr1 && len1) {
            FMOD_memset(ptr1, 0, len1);
            bytesreadtotal += len1;
        }

        if (ptr2 && len2) {
            FMOD_memset(ptr2, 0, len2);
            bytesreadtotal += len2;
        }

        result = this->unlock(ptr1, ptr2, len1, len2);
        if (result != FMOD_OK) {
            return result;
        }

        numbytes -= bytesreadtotal;
        offbytes += bytesreadtotal;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::loadSubSound(int32_t index, FMOD_MODE mode) {
    FMOD_RESULT result;
    FMOD_CODEC_WAVEFORMAT waveformat;
    Sample* sample = nullptr;

    if (!this->mNumSubSounds) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (index < 0 || index >= this->mNumSubSounds) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->mCodec->mDescription.getwaveformat(
        this->mCodec,
        index,
        &waveformat);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mSystem->createSample(
        mode,
        &waveformat,
        &sample);
    if (result != FMOD_OK) {
        return result;
    }

    sample->mType = this->mType;
    sample->mCodec = this->mCodec;

    if (this->mCodec->mDescription.soundcreate) {
        auto s = static_cast<SoundI*>(sample);

        result = this->mCodec->mDescription.soundcreate(
            this->mCodec,
            index,
            reinterpret_cast<FMOD_SOUND*>(s));
        if (result != FMOD_OK) {
            return result;
        }
    }

    this->mCodec->reset();

    result = this->mCodec->setPosition(index, 0, FMOD_TIMEUNIT_PCM);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mPostSetPositionCallback) {
        this->mPostSetPositionCallback(
            reinterpret_cast<FMOD_SOUND*>(this),
            index,
            0,
            FMOD_TIMEUNIT_PCM);
    }

    if (!(mode & FMOD_OPENONLY)) {
        result = sample->read(0, sample->mLength, 0);
        if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
            return result;
        }
    }

    result = this->setSubSound(index, sample);
    if (result != FMOD_OK) {
        return result;
    }

    sample->mPosition = 0;

    return FMOD_OK;
}

bool SoundI::isStream() {
    return false;
}

FMOD_RESULT SoundI::release(bool freethis) {
    FMOD_RESULT result;
    LocalCriticalSection crit(this->mSystem->mReadDataCrit);
    ChannelReal* oldstreamchannel;
    Codec* oldcodec;
    AsyncData* oldasyncdata;

    while ((this->mOpenState != FMOD_OPENSTATE_READY && this->mOpenState != FMOD_OPENSTATE_ERROR) || this->mFlags & FMOD_SOUND_FLAG_DONOTRELEASE) {
        FMOD_OS_Time_Sleep(2);
    }

    if (this->mCodec && this->mCodec->mFile) {
        this->mCodec->mFile->cancel();
    }

    if (this->mSystem) {
        result = this->mSystem->stopSound(this);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (this->isStream() && (this->mFlags & FMOD_SOUND_FLAG_READDATA) && !(this->mFlags & FMOD_SOUND_FLAG_PLAYED)) {
        crit.enter();
    }

    if (this->mSyncPointHead) {
        auto point = static_cast<SyncPoint*>(this->mSyncPointHead->getNext());
        while (point != this->mSyncPointHead) {
            this->deleteSyncPoint(reinterpret_cast<FMOD_SYNCPOINT*>(point));
            point = static_cast<SyncPoint*>(this->mSyncPointHead->getNext());
        }

        FMOD_Memory_Free(this->mSyncPointHead);
        this->mSyncPointHead = nullptr;
    }

    oldcodec = this->mCodec;
    oldasyncdata = this->mAsyncData;
    oldstreamchannel = nullptr;

    if (this->isStream() && this->mCodec && (!this->mSubSoundParent || this->mSubSoundParent == this || (this->mSubSoundParent && this->mCodec != this->mSubSoundParent->mCodec))) {
        auto stream = static_cast<Stream*>(this);

        if (stream->mChannel) {
            stream->mChannel->mFinished = true;
        }

        if (!(this->mSystem->mFlags & FMOD_INIT_STREAM_FROM_UPDATE) &&
            !stream->mStreamNode.isEmpty()) {
            while (!(stream->mFlags & FMOD_SOUND_FLAG_THREADFINISHED)) {
                FMOD_OS_Time_Sleep(2);
            }
        }

        FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamListCrit);
        stream->mStreamNode.removeNode();
        stream->mStreamNode.setData(nullptr);
        FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamListCrit);

        oldstreamchannel = stream->mChannel;

        if (stream->mSample) {
            stream->mSample->mCodec = nullptr;
            stream->mSample->release(true);
            stream->mSample = nullptr;
        }
    }

    if (this->mNumSubSounds && this->mSubSound) {
        if (this->mNumActiveSubSounds) {
            for (int32_t i = 0; i < this->mNumSubSounds; i++) {
                if (this->mSubSound[i]) {
                    if (this->mSubSound[i]->mCodec == oldcodec) {
                        this->mSubSound[i]->mCodec = nullptr;
                    }
                    if (this->mSubSound[i]->mAsyncData == oldasyncdata) {
                        this->mSubSound[i]->mAsyncData = nullptr;
                    }

                    if (this->mSubSound[i]->isStream()) {
                        auto stream = static_cast<Stream*>(this->mSubSound[i]);

                        if (stream->mChannel == oldstreamchannel) {
                            stream->mChannel = nullptr;
                        }
                    }

                    if (this->mSubSound[i]->mSubSoundShared) {
                        for (int32_t j = i + 1; j < this->mNumSubSounds; j++) {
                            if (this->mSubSound[j] == this->mSubSound[i]) {
                                this->mSubSound[j] = nullptr;
                            }
                        }
                    }

                    this->mSubSound[i]->release(true);
                    this->mSubSound[i] = nullptr;
                }
            }
        }

        FMOD_Memory_Free(this->mSubSound);
        this->mSubSound = nullptr;

        if (this->mSubSoundShared) {
            FMOD_Memory_Free(this->mSubSoundShared);
            this->mSubSoundShared = nullptr;
        }
    }

    if (this->mCodec) {
        if (!this->mSubSoundParent ||
            this->mSubSoundParent == this ||
            (this->mSubSoundParent && this->mCodec != this->mSubSoundParent->mCodec)) {

            this->mCodec->release();
            this->mCodec = nullptr;
        }
    }

    if (this->isStream()) {
        auto stream = static_cast<Stream*>(this);
        auto parentstream = static_cast<Stream*>(this->mSubSoundParent);

        if (parentstream && parentstream->mChannel == stream->mChannel) {
            stream->mChannel = nullptr;
        }

        if (stream->mChannel) {
            FMOD_Memory_Free(stream->mChannel);
            stream->mChannel = nullptr;
        }
    }

    if (this->mAsyncData) {
        if (!this->mSubSoundParent || this->mSubSoundParent->mAsyncData != this->mAsyncData) {
            FMOD_Memory_Free(this->mAsyncData);
        }
        this->mAsyncData = nullptr;
    }

    if (this->mSubSoundParent) {
        for (int32_t i = 0; i < this->mSubSoundParent->mNumSubSounds; i++) {
            if (this->mSubSoundParent->mSubSound && this->mSubSoundParent->mSubSound[i] == this) {
                this->mSubSoundParent->setSubSound(i, nullptr);
                break;
            }
        }
    }

    if (this->mSubSoundList) {
        FMOD_Memory_Free(this->mSubSoundList);
        this->mSubSoundList = nullptr;
    }

    if (this->mName) {
        FMOD_Memory_Free(this->mName);
        this->mName = nullptr;
    }

    FMOD_OS_CriticalSection_Enter(SystemI::gSoundListCrit);
    this->removeNode();
    this->mSoundGroupNode.removeNode();

    FMOD_OS_CriticalSection_Leave(SystemI::gSoundListCrit);

    if (freethis && !this->mSubSoundShared) {
        FMOD_Memory_Free(this);
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getSystemObject(System** system) {
    if (!system) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *system = reinterpret_cast<System*>(this->mSystem);
    return FMOD_OK;
}

FMOD_RESULT SoundI::lock(
    uint32_t offset,
    uint32_t length,
    void** ptr1,
    void** ptr2,
    uint32_t* len1,
    uint32_t* len2) {
    return FMOD_ERR_BADCOMMAND;
}

FMOD_RESULT SoundI::unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2) {
    return FMOD_ERR_BADCOMMAND;
}

FMOD_RESULT SoundI::setDefaults(
    float frequency,
    float volume,
    float pan,
    int32_t priority) {

    if (volume > 1.0f) {
        volume = 1.0f;
    }

    if (volume < 0.0f) {
        volume = 0.0f;
    }

    if (pan < -1.0f) {
        pan = -1.0f;
    }

    if (pan > 1.0f) {
        pan = 1.0f;
    }

    if (priority < 0) {
        priority = 0;
    }

    if (priority > 256) {
        priority = 256;
    }

    this->mDefaultFrequency = frequency;
    this->mDefaultVolume = volume;
    this->mDefaultPan = pan;
    this->mDefaultPriority = priority;

    return FMOD_OK;
}

FMOD_RESULT SoundI::getDefaults(
    float* frequency,
    float* volume,
    float* pan,
    int32_t* priority) {

    if (frequency) {
        *frequency = this->mDefaultFrequency;
    }

    if (volume) {
        *volume = this->mDefaultVolume;
    }

    if (pan) {
        *pan = this->mDefaultPan;
    }

    if (priority) {
        *priority = this->mDefaultPriority;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::setVariations(float frequencyvar, float volumevar, float panvar) {
    if (frequencyvar >= 0.0f) {
        this->mFrequencyVariation = frequencyvar;
    }

    if (volumevar >= 0.0f) {
        this->mVolumeVariation = volumevar;
    }

    if (panvar >= 0.0f) {
        this->mPanVariation = panvar;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getVariations(float* frequencyvar, float* volumevar, float* panvar) {
    if (frequencyvar) {
        *frequencyvar = this->mFrequencyVariation;
    }

    if (volumevar) {
        *volumevar = this->mVolumeVariation;
    }

    if (panvar) {
        *panvar = this->mPanVariation;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::set3DMinMaxDistance(float min, float max) {
    if (min < 0.0f || max < 0.0f || max < min) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mMinDistance = min;
    this->mMaxDistance = max;

    return FMOD_OK;
}

FMOD_RESULT SoundI::get3DMinMaxDistance(float* min, float* max) {
    if (min) {
        *min = this->mMinDistance;
    }

    if (max) {
        *max = this->mMaxDistance;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::set3DConeSettings(
    float insideconeangle,
    float outsideconeangle,
    float outsidevolume) {

    if (insideconeangle < 0.0f) {
        insideconeangle = 0.0f;
    }

    if (outsideconeangle < 0.0f) {
        outsideconeangle = 0.0f;
    }

    if (insideconeangle > 360.0f) {
        insideconeangle = 360.0f;
    }

    if (outsideconeangle > 360.0f) {
        outsideconeangle = 360.0f;
    }

    if (outsidevolume < 0.0f) {
        outsidevolume = 0.0f;
    }

    if (outsidevolume > 1.0f) {
        outsidevolume = 1.0f;
    }

    this->mConeInsideAngle = insideconeangle;
    this->mConeOutsideAngle = outsideconeangle;
    this->mConeOutsideVolume = outsidevolume;

    return FMOD_OK;
}

FMOD_RESULT SoundI::get3DConeSettings(
    float* insideconeangle,
    float* outsideconeangle,
    float* outsidevolume) {

    if (insideconeangle) {
        *insideconeangle = this->mConeInsideAngle;
    }

    if (outsideconeangle) {
        *outsideconeangle = this->mConeOutsideAngle;
    }

    if (outsidevolume) {
        *outsidevolume = this->mConeOutsideVolume;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::set3DCustomRolloff(FMOD_VECTOR* points, int32_t numpoints) {
    if (numpoints < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (points) {
        for (int32_t i = 1; i < numpoints; i++) {
            if (points[i].x <= points[i - 1].x) {
                return FMOD_ERR_INVALID_PARAM;
            }

            if (points[i].y < 0 || points[i].y > 1) {
                return FMOD_ERR_INVALID_PARAM;
            }
        }
    }

    this->mRolloffPoint = points;
    this->mNumRolloffPoints = numpoints;

    return FMOD_OK;
}

FMOD_RESULT SoundI::get3DCustomRolloff(FMOD_VECTOR** points, int32_t* numpoints) {
    if (points) {
        *points = this->mRolloffPoint;
    }

    if (numpoints) {
        *numpoints = this->mNumRolloffPoints;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::setSubSound(int32_t index, SoundI* subsound) {
    if (index < 0 || this->mNumSubSounds <= index) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (subsound && subsound->mSubSoundParent) {
        return FMOD_ERR_SUBSOUND_ALLOCATED;
    }

    bool blockstream = false;

    if (this->isStream()) {
        // auto stream = static_cast<Stream*>(this);
        auto soundi = this;

        if (soundi->mSubSoundShared) {
            soundi = soundi->mSubSoundShared;
        } else if (soundi->mSubSound) {
            soundi = soundi->mSubSound[soundi->mSubSoundIndex];
        }

        if (soundi && soundi->mFlags & FMOD_SOUND_FLAG_PLAYED && !(soundi->mFlags & FMOD_SOUND_FLAG_FINISHED)) {
            blockstream = true;
        }
    }

    if (blockstream) {
        FMOD_OS_CriticalSection_Enter(this->mSystem->mStreamUpdateCrit);
    }

    auto oldsubsound = this->mSubSound[index];

    if (subsound) {
        if (!subsound->isStream()) {
            auto sample = static_cast<Sample*>(subsound);
            for (int32_t i = 0; i < sample->mNumSubSamples; i++) {
                sample->mSubSample[i]->mCodec = this->mCodec;

                if (oldsubsound) {
                    auto oldsample = static_cast<Sample*>(oldsubsound);
                    sample->mSubSample[i]->mSubSampleParent = oldsample->mSubSampleParent;
                }
            }
        }

        subsound->mSubSoundIndex = index;
        subsound->mSubSoundParent = this;
    }

    this->mSubSound[index] = subsound;

    if (oldsubsound) {
        oldsubsound->mSubSoundParent = nullptr;

        if (!subsound) {
            this->mNumActiveSubSounds--;
        }
    } else {
        if (subsound) {
            this->mNumActiveSubSounds++;
        }
    }

    if (this->mSubSoundListNum) {
        this->mLength = 0;

        for (int32_t i = 0; i < this->mSubSoundListNum; i++) {
            if (this->mSubSound[this->mSubSoundList[i]]) {
                this->mLength += this->mSubSound[this->mSubSoundList[i]]->mLength;
            }
        }
    }

    this->mLoopStart = 0;
    this->mLoopLength = this->mLength;

    if (this->isStream()) {
        if (subsound) {
            this->mFlags |= FMOD_SOUND_FLAG_WANTSTOFLUSH;
        }

        if (blockstream) {
            FMOD_OS_CriticalSection_Leave(this->mSystem->mStreamUpdateCrit);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getSubSound(int32_t index, SoundI** subsound) {
    if (!subsound) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *subsound = nullptr;

    if (index < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (index >= this->mNumSubSounds) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *subsound = this->mSubSound[index];

    if (this->mSubSound[index] && mMode & FMOD_NONBLOCKING && this->mSubSound[index]->isStream()) {
        if (this->mSubSound[index]->mOpenState == FMOD_OPENSTATE_SEEKING) {
            return FMOD_ERR_NOTREADY;
        }
    }

    if (this->mSubSound[index] && this->mSubSound[index]->mSubSoundShared) {
        this->mSubSound[index]->updateSubSound(index, false);
    }

    if (this->mSubSound[index] && this->mSubSound[index]->isStream()) {
        auto stream = static_cast<Stream*>(*subsound);

        if (this->mMode & FMOD_NONBLOCKING) {
            FMOD_RESULT result;
            auto streamParent = static_cast<Stream*>(stream->mSubSoundParent);

            if ((index == stream->mSubSoundParent->mSubSoundIndex &&
                 (stream->mPosition == streamParent->mInitialPosition + stream->mSample->mLength ||
                  (stream->mLength < stream->mSample->mLength && stream->mPosition == streamParent->mInitialPosition + stream->mLength))) &&
                !(stream->mFlags & FMOD_SOUND_FLAG_WANTSTOFLUSH)) {
                return FMOD_OK;
            }

            stream->mOpenState = FMOD_OPENSTATE_SEEKING;
            this->mOpenState = FMOD_OPENSTATE_SEEKING;

            result = AsyncThread::getAsyncThread(stream);
            if (result != FMOD_OK) {
                return result;
            }

            FMOD_OS_CriticalSection_Enter(stream->mAsyncData->mThread->mCrit);
            stream->mAsyncData->mNode.setData(stream);
            stream->mAsyncData->mNode.addBefore(&stream->mAsyncData->mThread->mHead);
            FMOD_OS_CriticalSection_Leave(stream->mAsyncData->mThread->mCrit);

            stream->mAsyncData->mThread->mThread.wakeupThread();
        } else {
            if (stream->mSubSoundIndex != stream->mCodec->mSubSoundIndex) {
                FMOD_RESULT result;

                result = stream->setPosition(0, FMOD_TIMEUNIT_PCM);
                if (result == FMOD_OK) {
                    stream->flush();
                }
            }
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::setSubSoundSentence(int32_t* subsoundlist, int32_t numsubsounds) {
    if (!this->mNumSubSounds) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (subsoundlist && !numsubsounds) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->isStream()) {
        return FMOD_ERR_FORMAT;
    }

    if (subsoundlist) {
        for (int32_t i = 0; i < numsubsounds; i++) {
            if (subsoundlist[i] < 0 || subsoundlist[i] >= this->mNumSubSounds) {
                return FMOD_ERR_INVALID_PARAM;
            }

            auto subsound = this->mSubSound[subsoundlist[i]];
            if (!subsound) {
                return FMOD_ERR_INVALID_PARAM;
            }

            if (subsound->isStream() != this->isStream()) {
                return FMOD_ERR_FORMAT;
            }

            if (this->isStream()) {
                if (this->mFormat != subsound->mFormat) {
                    return FMOD_ERR_FORMAT;
                }

                if (this->mChannels != subsound->mChannels) {
                    return FMOD_ERR_FORMAT;
                }
            }
        }
    }

    if (this->mSubSoundList) {
        FMOD_Memory_Free(this->mSubSoundList);
    }

    this->mSubSoundListNum = numsubsounds;

    if (this->mSubSoundListNum) {
        this->mSubSoundList = static_cast<int32_t*>(FMOD_Memory_Calloc(sizeof(int32_t) * numsubsounds));
        if (!this->mSubSoundList) {
            return FMOD_ERR_MEMORY;
        }

        this->mLength = 0;
        for (int32_t i = 0; i < this->mSubSoundListNum; i++) {
            if (subsoundlist) {
                this->mSubSoundList[i] = subsoundlist[i];
            } else {
                this->mSubSoundList[i] = i;
            }

            if (this->mSubSound[this->mSubSoundList[i]]) {
                this->mLength += this->mSubSound[this->mSubSoundList[i]]->mLength;
            }
        }
    }

    this->mLoopStart = 0;
    this->mLoopLength = this->mLength;
    this->mSubSoundListCurrent = 0;

    if (this->isStream()) {
        auto stream = static_cast<Stream*>(this);
        stream->mCodec = this->mSubSound[this->mSubSoundList[this->mSubSoundListCurrent]]->mCodec;
        stream->mFlags |= FMOD_SOUND_FLAG_WANTSTOFLUSH;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getName(char* name, int32_t namelen) {
    if (!name) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (namelen > FMOD_STRING_MAXNAMELEN) {
        namelen = FMOD_STRING_MAXNAMELEN;
    }

    if (this->mName) {
        FMOD_strncpy(name, this->mName, namelen);
    } else {
        FMOD_strncpy(name, "(null)", namelen);
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getLength(uint32_t* length, FMOD_TIMEUNIT lengthtype) {
    if (!length) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (lengthtype == FMOD_TIMEUNIT_SENTENCE) {
        *length = this->mSubSoundListNum;
    } else if (lengthtype == FMOD_TIMEUNIT_PCM) {
        *length = this->mLength;
    } else if (lengthtype == FMOD_TIMEUNIT_MS) {
        if (!this->mDefaultFrequency) {
            *length = static_cast<uint32_t>(-1);
        } else if (this->mLength == 0xFFFFFFFF) {
            *length = static_cast<uint32_t>(-1);
        } else {
            *length = static_cast<uint32_t>(static_cast<FMOD_UINT64>(this->mLength) * static_cast<FMOD_UINT64>(1000) / static_cast<FMOD_UINT64>(this->mDefaultFrequency));
        }
    } else if (lengthtype == FMOD_TIMEUNIT_PCMBYTES) {
        if (this->mLength == 0xFFFFFFFF) {
            *length = static_cast<uint32_t>(-1);
        } else if (this->mMode & FMOD_CREATECOMPRESSEDSAMPLE) {
            *length = this->mLength * sizeof(int16_t) * this->mChannels;
        } else {
            this->getBytesFromSamples(this->mLength, length);
        }
    } else {
        if (!this->mCodec) {
            return FMOD_ERR_INVALID_PARAM;
        }

        return this->mCodec->getLength(length, lengthtype);
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getFormat(
    FMOD_SOUND_TYPE* type,
    FMOD_SOUND_FORMAT* format,
    int32_t* channels,
    int32_t* bits) {

    if (type) {
        *type = this->mType;
    }

    if (format) {
        *format = this->mFormat;
    }

    if (channels) {
        *channels = this->mChannels;
    }

    if (bits) {
        SoundI::getBitsFromFormat(bits);
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getNumSubSounds(int32_t* numsubsounds) {
    if (!numsubsounds) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numsubsounds = this->mNumSubSounds;

    return FMOD_OK;
}

FMOD_RESULT SoundI::getNumTags(int32_t* numtags, int32_t* numtagsupdated) {
    if (!numtags && !numtagsupdated) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (numtags) {
        *numtags = 0;
    }
    if (numtagsupdated) {
        *numtagsupdated = 0;
    }

    if (this->mCodec && this->mCodec->mMetadata) {
        return this->mCodec->mMetadata->getNumTags(numtags, numtagsupdated);
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getTag(const char* name, int32_t index, FMOD_TAG* tag) {
    if (!tag) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!this->mCodec || !this->mCodec->mMetadata) {
        return FMOD_ERR_TAGNOTFOUND;
    }

    return mCodec->mMetadata->getTag(name, index, tag);
}

FMOD_RESULT SoundI::getOpenState(
    FMOD_OPENSTATE* openstate,
    uint32_t* percentbuffered,
    bool* starving) {

    if (openstate) {
        *openstate = this->mOpenState;
        if (this->mOpenState == FMOD_OPENSTATE_READY && this->mFlags & FMOD_SOUND_FLAG_DONOTRELEASE) {
            *openstate = FMOD_OPENSTATE_BUFFERING;
        }
    }

    if (percentbuffered) {
        if (this->mCodec &&
            this->mCodec->mFile &&
            (this->mOpenState == FMOD_OPENSTATE_BUFFERING ||
             this->mOpenState == FMOD_OPENSTATE_READY)) {
            *percentbuffered = this->mCodec->mFile->mPercentBuffered;
        } else {
            *percentbuffered = 0;
        }
    }

    if (starving) {
        *starving = false;

        if (this->mCodec && this->mCodec->mFile) {
            *starving = this->mCodec->mFile->mStarving;
        }
    }

    if (this->mAsyncData) {
        return this->mAsyncData->mResult;
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::readData(void* buffer, uint32_t numbytes, uint32_t* read) {
    FMOD_CODEC_WAVEFORMAT waveformat;
    FMOD_RESULT result;
    FMOD_UINT_NATIVE threadid;
    LocalCriticalSection crit(this->mSystem->mReadDataCrit);
    uint32_t offbytes;
    uint32_t readbytes;
    uint32_t scale;
    uint32_t position = 0;
    uint32_t chunksize;
    uint32_t blockalign;
    int32_t bits;

    if (!this->mCodec) {
        return FMOD_ERR_PLUGIN_MISSING;
    }

    if (!buffer) {
        return FMOD_ERR_INVALID_PARAM;
    }

    result = this->getBitsFromFormat(&bits);
    if (result != FMOD_OK) {
        return result;
    }

    if (!this->mCodec->mFile) {
        return FMOD_ERR_UNSUPPORTED;
    }

    FMOD_OS_Thread_GetCurrentID(&threadid);

    if (threadid != this->mSystem->mMainThreadID) {
        this->mFlags |= (FMOD_SOUND_FLAG_DONOTRELEASE | FMOD_SOUND_FLAG_READDATA);
        crit.enter();
    }

    result = this->mCodec->mDescription.getwaveformat(
        this->mCodec,
        this->mCodec->mSubSoundIndex,
        &waveformat);
    if (result != FMOD_OK) {
        mFlags &= ~FMOD_SOUND_FLAG_DONOTRELEASE;
        return result;
    }

    offbytes = 0;
    readbytes = 0;

    scale = 1;
    if (this->mChannels != waveformat.channels && this->mChannels == 1) {
        scale = waveformat.channels;
    }

    chunksize = SOUND_READCHUNKSIZE;

    blockalign = 1;
    if (this->mMode & FMOD_CREATESTREAM) {
        if (this->mCodec->mBlockAlign) {
            blockalign = this->mCodec->mBlockAlign;
        }
    } else {
        if (waveformat.blockalign) {
            blockalign = waveformat.blockalign;
        }
    }

    chunksize /= blockalign;
    chunksize *= blockalign;
    if (!chunksize) {
        chunksize = blockalign;
    }

    while (numbytes) {
        uint32_t size = numbytes > (chunksize / scale) ? (chunksize / scale) : numbytes;
        uint32_t bytesreadtotal = 0;
        uint32_t bytesread = 0;
        void* ptr;

        ptr = reinterpret_cast<char*>(buffer) + offbytes;

        if (this->mMode & FMOD_CREATECOMPRESSEDSAMPLE) {
            result = this->mCodec->mFile->read(ptr, 1, size, &bytesread);
            if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
                mFlags &= ~FMOD_SOUND_FLAG_DONOTRELEASE;
                return result;
            }
        } else {
            result = this->mCodec->read(ptr, size, &bytesread);
            if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
                mFlags &= ~FMOD_SOUND_FLAG_DONOTRELEASE;
                return result;
            }
        }

        if (this->mPostReadCallback && bytesread) {
            this->mPostReadCallback(this->mPostCallbackSound, ptr, bytesread);
        }

        bytesreadtotal += (bytesread / scale);

        if (result == FMOD_ERR_FILE_EOF) {
            numbytes = bytesreadtotal;
        }

        numbytes -= bytesreadtotal;
        offbytes += bytesreadtotal;
        readbytes += bytesreadtotal;

        this->getSamplesFromBytes(bytesreadtotal, &position);
        this->mPosition += position;

        if (this->mPosition > this->mLength) {
            this->mPosition = this->mLength;
        }
    }

    if (read) {
        *read = readbytes;
    }

    this->mFlags &= ~FMOD_SOUND_FLAG_DONOTRELEASE;

    return result;
}

FMOD_RESULT SoundI::seekData(uint32_t position) {
    if (this->mCodec) {
        this->mCodec->reset();
    }

    return this->seek(this->mSubSoundIndex, position);
}

FMOD_RESULT SoundI::setSoundGroup(SoundGroupI* soundgroup) {
    this->mSoundGroup = soundgroup;
    if (!this->mSoundGroup) {
        this->mSoundGroup = this->mSystem->mSoundGroup;
    }

    FMOD_OS_CriticalSection_Enter(SystemI::gSoundListCrit);
    this->mSoundGroupNode.removeNode();
    this->mSoundGroupNode.addBefore(&this->mSoundGroup->mSoundHead);
    this->mSoundGroupNode.setData(this);
    FMOD_OS_CriticalSection_Leave(SystemI::gSoundListCrit);

    this->mSoundGroup->removeNode();
    this->mSoundGroup->addAfter(&this->mSystem->mSoundGroupUsedHead);

    return FMOD_OK;
}

FMOD_RESULT SoundI::getSoundGroup(SoundGroupI** soundgroup) {
    if (!soundgroup) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *soundgroup = this->mSoundGroup;
    return FMOD_OK;
}

FMOD_RESULT SoundI::getNumSyncPoints(int32_t* numsyncpoints) {
    if (!numsyncpoints) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *numsyncpoints = this->mNumSyncPoints;
    return FMOD_OK;
}

FMOD_RESULT SoundI::getSyncPoint(int32_t index, FMOD_SYNCPOINT** point) {
    if (index < 0 || index >= this->mNumSyncPoints || !point) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto current = static_cast<SyncPoint*>(this->mSyncPointHead->getNext());

    for (; index; index--) {
        current = static_cast<SyncPoint*>(current->getNext());
    }

    *point = reinterpret_cast<FMOD_SYNCPOINT*>(current);
    return FMOD_OK;
}

FMOD_RESULT SoundI::getSyncPointInfo(
    FMOD_SYNCPOINT* point,
    char* name,
    int32_t namelen,
    uint32_t* offset,
    FMOD_TIMEUNIT offsettype) {
    auto spoint = reinterpret_cast<SyncPoint*>(point);

    if (!point) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (!name && !offset) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (name) {
        FMOD_strncpy(name, spoint->mName, namelen);
    }

    if (offset) {
        if (offsettype == FMOD_TIMEUNIT_PCM) {
            *offset = spoint->mOffset;
        } else if (offsettype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getBytesFromSamples(spoint->mOffset, offset, this->mChannels, this->mFormat);
        } else if (offsettype == FMOD_TIMEUNIT_MS) {
            *offset = static_cast<uint32_t>(static_cast<float>(spoint->mOffset) * 1000.0f / this->mDefaultFrequency);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::addSyncPoint(
    uint32_t offset,
    FMOD_TIMEUNIT offsettype,
    const char* name,
    FMOD_SYNCPOINT** syncpoint,
    int32_t subsound,
    bool fixupindicies) {

    SyncPoint* point;
    uint32_t pcm;

    if (!this->mSyncPointHead) {
        this->mSyncPointHead = FMOD_Object_Alloc(SyncPoint);
        if (!mSyncPointHead) {
            return FMOD_ERR_MEMORY;
        }

        this->mSyncPointHead->mOffset = static_cast<uint32_t>(-1);
        FMOD_strcpy(this->mSyncPointHead->mName, "head");
    }

    point = static_cast<SyncPoint*>(FMOD_Memory_Alloc(sizeof(SyncPoint)));
    if (!point) {
        return FMOD_ERR_MEMORY;
    }

    new (point) SyncPoint;

    if (syncpoint) {
        *syncpoint = reinterpret_cast<FMOD_SYNCPOINT*>(point);
    }

    if (offsettype == FMOD_TIMEUNIT_PCM) {
        pcm = offset;
    } else if (offsettype == FMOD_TIMEUNIT_PCMBYTES) {
        SoundI::getSamplesFromBytes(offset, &pcm, this->mChannels, this->mFormat);
    } else if (offsettype == FMOD_TIMEUNIT_MS) {
        pcm = static_cast<uint32_t>(static_cast<float>(offset) / 1000.0f * this->mDefaultFrequency);
    } else {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(point->mName, name, FMOD_STRING_MAXNAMELEN);

    point->mOffset = pcm;

    point->addAt(this->mSyncPointHead, pcm);
    this->mNumSyncPoints++;

    return FMOD_OK;
}

FMOD_RESULT SoundI::deleteSyncPoint(FMOD_SYNCPOINT* point) {
    auto syncpoint = reinterpret_cast<SyncPoint*>(point);
    if (!syncpoint) {
        return FMOD_ERR_INVALID_PARAM;
    }

    syncpoint->removeNode();
    FMOD_Memory_Free(syncpoint);
    this->mNumSyncPoints--;
    return FMOD_OK;
}

FMOD_RESULT SoundI::setMode(FMOD_MODE mode) {
    if (mode & (FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI)) {
        this->mMode &= ~(FMOD_LOOP_OFF | FMOD_LOOP_NORMAL | FMOD_LOOP_BIDI);

        if (mode & FMOD_LOOP_OFF) {
            this->mMode |= FMOD_LOOP_OFF;
        } else if (mode & FMOD_LOOP_NORMAL) {
            this->mMode |= FMOD_LOOP_NORMAL;
        } else if (mode & FMOD_LOOP_BIDI) {
            this->mMode |= FMOD_LOOP_BIDI;
        }

        if (this->isStream()) {
            this->mFlags &= ~FMOD_SOUND_FLAG_FINISHED;
        }
    }

    if (mode & FMOD_3D_HEADRELATIVE) {
        this->mMode &= ~FMOD_3D_WORLDRELATIVE;
        this->mMode |= FMOD_3D_HEADRELATIVE;
    } else if (mode & FMOD_3D_WORLDRELATIVE) {
        this->mMode &= ~FMOD_3D_HEADRELATIVE;
        this->mMode |= FMOD_3D_WORLDRELATIVE;
    }

    if (mode & FMOD_3D_LOGROLLOFF) {
        this->mMode &= ~FMOD_3D_LINEARROLLOFF;
        this->mMode &= ~FMOD_3D_CUSTOMROLLOFF;
        this->mMode |= FMOD_3D_LOGROLLOFF;
    } else if (mode & FMOD_3D_LINEARROLLOFF) {
        this->mMode &= ~FMOD_3D_LOGROLLOFF;
        this->mMode &= ~FMOD_3D_CUSTOMROLLOFF;
        this->mMode |= FMOD_3D_LINEARROLLOFF;
    } else if (mode & FMOD_3D_CUSTOMROLLOFF) {
        this->mMode &= ~FMOD_3D_LOGROLLOFF;
        this->mMode &= ~FMOD_3D_LINEARROLLOFF;
        this->mMode |= FMOD_3D_CUSTOMROLLOFF;
    }

    if (mode & FMOD_3D_IGNOREGEOMETRY) {
        this->mMode |= FMOD_3D_IGNOREGEOMETRY;
    } else {
        this->mMode &= ~FMOD_3D_IGNOREGEOMETRY;
    }

    if (mode & FMOD_VIRTUAL_PLAYFROMSTART) {
        this->mMode |= FMOD_VIRTUAL_PLAYFROMSTART;
    } else {
        this->mMode &= ~FMOD_VIRTUAL_PLAYFROMSTART;
    }

    if (!(this->mMode & FMOD_HARDWARE)) {
        if (mode & FMOD_2D) {
            this->mMode &= ~FMOD_3D;
            this->mMode |= FMOD_2D;
        } else if (mode & FMOD_3D) {
            this->mMode &= ~FMOD_2D;
            this->mMode |= FMOD_3D;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::getMode(FMOD_MODE* mode) {
    if (!mode) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *mode = this->mMode;
    return FMOD_OK;
}

FMOD_RESULT SoundI::setLoopCount(int32_t loopcount) {
    FMOD_RESULT result;

    if (loopcount) {
        result = this->setMode(FMOD_LOOP_NORMAL);
    } else {
        result = this->setMode(FMOD_LOOP_OFF);
    }

    if (result != FMOD_OK) {
        return result;
    }

    this->mLoopCount = loopcount;
    return FMOD_OK;
}

FMOD_RESULT SoundI::getLoopCount(int32_t* loopcount) {
    if (loopcount) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *loopcount = this->mLoopCount;
    return FMOD_OK;
}

FMOD_RESULT SoundI::setLoopPoints(
    uint32_t loopstart,
    FMOD_TIMEUNIT loopstarttype,
    uint32_t loopend,
    FMOD_TIMEUNIT loopendtype) {

    uint32_t loopstartpcm = 0;
    uint32_t loopendpcm = 0;

    if ((loopstarttype != FMOD_TIMEUNIT_MS && loopstarttype != FMOD_TIMEUNIT_PCM && loopstarttype != FMOD_TIMEUNIT_PCMBYTES) ||
        (loopendtype != FMOD_TIMEUNIT_MS && loopendtype != FMOD_TIMEUNIT_PCM && loopendtype != FMOD_TIMEUNIT_PCMBYTES)) {
        return FMOD_ERR_FORMAT;
    }

    if (loopstarttype == FMOD_TIMEUNIT_PCM) {
        loopstartpcm = loopstart;
    } else if (loopstarttype == FMOD_TIMEUNIT_PCMBYTES) {
        SoundI::getSamplesFromBytes(loopstart, &loopstartpcm, this->mChannels, this->mFormat);
    } else if (loopstarttype == FMOD_TIMEUNIT_MS) {
        loopstartpcm = static_cast<uint32_t>(static_cast<float>(loopstart) / 1000.0f * this->mDefaultFrequency);
    }

    if (loopendtype == FMOD_TIMEUNIT_PCM) {
        loopendpcm = loopend;
    } else if (loopendtype == FMOD_TIMEUNIT_PCMBYTES) {
        SoundI::getSamplesFromBytes(loopend, &loopendpcm, this->mChannels, this->mFormat);
    } else if (loopendtype == FMOD_TIMEUNIT_MS) {
        loopendpcm = static_cast<uint32_t>(static_cast<float>(loopend) / 1000.0f * this->mDefaultFrequency);
    }

    if (loopstartpcm >= this->mLength) {
        loopstartpcm = 0;
    }
    if (loopendpcm >= this->mLength) {
        loopendpcm = this->mLength - 1;
    }

    if (!loopendpcm) {
        loopendpcm = this->mLength - 1;
    }

    if (loopstartpcm >= loopendpcm) {
        return FMOD_ERR_INVALID_PARAM;
    }

    this->mLoopStart = loopstartpcm;
    this->mLoopLength = loopendpcm - loopstartpcm + 1;

    return FMOD_OK;
}

FMOD_RESULT SoundI::getLoopPoints(
    uint32_t* loopstart,
    FMOD_TIMEUNIT loopstarttype,
    uint32_t* loopend,
    FMOD_TIMEUNIT loopendtype) {

    if ((loopstarttype != FMOD_TIMEUNIT_MS && loopstarttype != FMOD_TIMEUNIT_PCM && loopstarttype != FMOD_TIMEUNIT_PCMBYTES) ||
        (loopendtype != FMOD_TIMEUNIT_MS && loopendtype != FMOD_TIMEUNIT_PCM && loopendtype != FMOD_TIMEUNIT_PCMBYTES)) {
        return FMOD_ERR_FORMAT;
    }

    if (loopstart) {
        if (loopstarttype == FMOD_TIMEUNIT_PCM) {
            *loopstart = this->mLoopStart;
        } else if (loopstarttype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getBytesFromSamples(this->mLoopStart, loopstart, this->mChannels, this->mFormat);
        } else if (loopstarttype == FMOD_TIMEUNIT_MS) {
            *loopstart = static_cast<uint32_t>(static_cast<float>(this->mLoopStart) * 1000.0f / this->mDefaultFrequency);
        }
    }

    if (loopend) {
        uint32_t lend = this->mLoopStart + this->mLoopLength - 1;

        if (loopendtype == FMOD_TIMEUNIT_PCM) {
            *loopend = lend;
        } else if (loopendtype == FMOD_TIMEUNIT_PCMBYTES) {
            SoundI::getBytesFromSamples(lend, loopend, this->mChannels, this->mFormat);
        } else if (loopendtype == FMOD_TIMEUNIT_MS) {
            *loopend = static_cast<uint32_t>(static_cast<float>(lend) * 1000.0f / this->mDefaultFrequency);
        }
    }

    return FMOD_OK;
}

FMOD_RESULT SoundI::setPosition(uint32_t pos) {
    this->mPosition = pos;
    return FMOD_OK;
}

FMOD_RESULT SoundI::getPosition(uint32_t* pcm) {
    if (!pcm) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *pcm = this->mPosition;
    return FMOD_OK;
}

FMOD_RESULT SoundI::setUserData(void* userdata) {
    this->mUserData = userdata;
    return FMOD_OK;
}

FMOD_RESULT SoundI::getUserData(void** userdata) {
    if (!userdata) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *userdata = this->mUserData;
    return FMOD_OK;
}

FMOD_RESULT SoundI::validate(Sound* sound, SoundI** soundi) {
    if (!soundi) {
        return FMOD_ERR_INVALID_PARAM;
    }
    if (!sound) {
        return FMOD_ERR_INVALID_HANDLE;
    }

    *soundi = reinterpret_cast<SoundI*>(sound);
    return FMOD_OK;
}

} // namespace FMOD
