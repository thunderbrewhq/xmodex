#ifndef XMODEX_SOUND_SOUND_I_HPP
#define XMODEX_SOUND_SOUND_I_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"
#include "xmodex/Types.hpp"

#define FMOD_SOUND_FLAG_DONOTRELEASE 0x00000001
#define FMOD_SOUND_FLAG_READDATA 0x00000002
#define FMOD_SOUND_FLAG_PLAYED 0x00000004
#define FMOD_SOUND_FLAG_FINISHED 0x00000008
#define FMOD_SOUND_FLAG_WANTSTOFLUSH 0x00000010
#define FMOD_SOUND_FLAG_RELEASING 0x00000020
#define FMOD_SOUND_FLAG_THREADFINISHED 0x00000040
#define FMOD_SOUND_FLAG_FULLYBUFFERED 0x00000080
#define FMOD_SOUND_FLAG_PRELOADEDFSB 0x00000100
#define FMOD_SOUND_FLAG_PROGRAMMERSOUND 0x00000200
#define FMOD_SOUND_FLAG_SETPOS_SAFE 0x00000400

namespace FMOD {

class AsyncData;
class Codec;
class SoundGroupI;
class SyncPoint;
class SystemI;

typedef uint32_t FMOD_SOUND_FLAG;

const uint32_t SOUND_READCHUNKSIZE = 16384;

class SoundI : public LinkedListNode {
    public:
        FMOD_SOUND_TYPE mType;
        FMOD_SOUND_FORMAT mFormat;
        FMOD_MODE mMode;
        char* mName;
        uint32_t mPosition;
        uint32_t mLength;
        uint32_t mLengthBytes;
        uint32_t mLoopStart;
        uint32_t mLoopLength;
        int32_t mLoopCount;
        FMOD_SOUND_FLAG mFlags;
        Codec* mCodec;
        int32_t mChannels;
        float mDefaultVolume;
        float mDefaultFrequency;
        float mDefaultPan;
        int32_t mDefaultPriority;
        uint32_t mDefaultChannelMask;
        float mFrequencyVariation;
        float mVolumeVariation;
        float mPanVariation;
        float mMinDistance;
        float mMaxDistance;
        float mConeInsideAngle;
        float mConeOutsideAngle;
        float mConeOutsideVolume;
        FMOD_VECTOR* mRolloffPoint;
        int32_t mNumRolloffPoints;
        SoundI** mSubSound;
        SoundI* mSubSoundShared;
        int32_t mNumSubSounds;
        int32_t mNumActiveSubSounds;
        SoundI* mSubSoundParent;
        int32_t mSubSoundIndex;
        int32_t* mSubSoundList;
        int32_t mSubSoundListNum;
        int32_t mSubSoundListCurrent;
        SoundI* mSubSampleParent;
        void* mUserData;
        SystemI* mSystem;
        uint32_t mMemoryUsed;
        int32_t mNumSyncPoints;
        SyncPoint* mSyncPointHead;
        AsyncData* mAsyncData;
        FMOD_OPENSTATE mOpenState;
        SoundGroupI* mSoundGroup;
        LinkedListNode mSoundGroupNode;
        int32_t mNumAudible;
        FMOD_SOUND_PCMREADCALLBACK mPostReadCallback;
        FMOD_SOUND_PCMSETPOSCALLBACK mPostSetPositionCallback;
        FMOD_SOUND* mPostCallbackSound;

    public:
        SoundI();

        bool readyForRelease();

        FMOD_RESULT seek(int32_t subsound, uint32_t position);
        FMOD_RESULT updateSubSound(int32_t index, bool updateinfoonly);
        FMOD_RESULT read(uint32_t offset, uint32_t numsamples, uint32_t* read);
        FMOD_RESULT clear(uint32_t offset, uint32_t numsamples);
        FMOD_RESULT loadSubSound(int32_t index, FMOD_MODE mode);

        // virtual SoundI functions
        virtual bool isStream();
        virtual FMOD_RESULT release(bool freethis);
        virtual FMOD_RESULT getSystemObject(System** system);
        virtual FMOD_RESULT lock(
            uint32_t offset,
            uint32_t length,
            void** ptr1,
            void** ptr2,
            uint32_t* len1,
            uint32_t* len2);
        virtual FMOD_RESULT unlock(void* ptr1, void* ptr2, uint32_t len1, uint32_t len2);
        virtual FMOD_RESULT setDefaults(
            float frequency,
            float volume,
            float pan,
            int32_t priority);
        virtual FMOD_RESULT getDefaults(
            float* frequency,
            float* volume,
            float* pan,
            int32_t* priority);
        virtual FMOD_RESULT setVariations(float frequencyvar, float volumevar, float panvar);
        virtual FMOD_RESULT getVariations(float* frequencyvar, float* volumevar, float* panvar);
        virtual FMOD_RESULT set3DMinMaxDistance(float min, float max);
        virtual FMOD_RESULT get3DMinMaxDistance(float* min, float* max);
        virtual FMOD_RESULT set3DConeSettings(
            float insideconeangle,
            float outsideconeangle,
            float outsidevolume);
        virtual FMOD_RESULT get3DConeSettings(
            float* insideconeangle,
            float* outsideconeangle,
            float* outsidevolume);
        virtual FMOD_RESULT set3DCustomRolloff(FMOD_VECTOR* points, int32_t numpoints);
        virtual FMOD_RESULT get3DCustomRolloff(FMOD_VECTOR** points, int32_t* numpoints);
        virtual FMOD_RESULT setSubSound(int32_t index, SoundI* subsound);
        virtual FMOD_RESULT getSubSound(int32_t index, SoundI** subsound);
        virtual FMOD_RESULT setSubSoundSentence(int32_t* subsoundlist, int32_t numsubsounds);
        virtual FMOD_RESULT getName(char* name, int32_t namelen);
        virtual FMOD_RESULT getLength(uint32_t* length, FMOD_TIMEUNIT lengthtype);
        virtual FMOD_RESULT getFormat(
            FMOD_SOUND_TYPE* type,
            FMOD_SOUND_FORMAT* format,
            int32_t* channels,
            int32_t* bits);
        virtual FMOD_RESULT getNumSubSounds(int32_t* numsubsounds);
        virtual FMOD_RESULT getNumTags(int32_t* numtags, int32_t* numtagsupdated);
        virtual FMOD_RESULT getTag(const char* name, int32_t index, FMOD_TAG* tag);
        virtual FMOD_RESULT getOpenState(
            FMOD_OPENSTATE* openstate,
            uint32_t* percentbuffered,
            bool* starving);
        virtual FMOD_RESULT readData(void* buffer, uint32_t numbytes, uint32_t* read);
        virtual FMOD_RESULT seekData(uint32_t position);
        virtual FMOD_RESULT setSoundGroup(SoundGroupI* soundgroup);
        virtual FMOD_RESULT getSoundGroup(SoundGroupI** soundgroup);
        virtual FMOD_RESULT getNumSyncPoints(int32_t* numsyncpoints);
        virtual FMOD_RESULT getSyncPoint(int32_t index, FMOD_SYNCPOINT** point);
        virtual FMOD_RESULT getSyncPointInfo(
            FMOD_SYNCPOINT* point,
            char* name,
            int32_t namelen,
            uint32_t* offset,
            FMOD_TIMEUNIT offsettype);
        virtual FMOD_RESULT addSyncPoint(
            uint32_t offset,
            FMOD_TIMEUNIT offsettype,
            const char* name,
            FMOD_SYNCPOINT** syncpoint,
            int32_t subsound,
            bool fixupindicies);
        virtual FMOD_RESULT deleteSyncPoint(FMOD_SYNCPOINT* point);
        virtual FMOD_RESULT setMode(FMOD_MODE mode);
        virtual FMOD_RESULT getMode(FMOD_MODE* mode);
        virtual FMOD_RESULT setLoopCount(int32_t loopcount);
        virtual FMOD_RESULT getLoopCount(int32_t* loopcount);
        virtual FMOD_RESULT setLoopPoints(
            uint32_t loopstart,
            FMOD_TIMEUNIT loopstarttype,
            uint32_t loopend,
            FMOD_TIMEUNIT loopendtype);
        virtual FMOD_RESULT getLoopPoints(
            uint32_t* loopstart,
            FMOD_TIMEUNIT loopstarttype,
            uint32_t* loopend,
            FMOD_TIMEUNIT loopendtype);
        virtual FMOD_RESULT setPosition(uint32_t pos);
        virtual FMOD_RESULT getPosition(uint32_t* pcm);
        virtual FMOD_RESULT setUserData(void* userdata);
        virtual FMOD_RESULT getUserData(void** userdata);

        inline FMOD_RESULT getSamplesFromBytes(uint32_t bytes, uint32_t* samples) {
            return this->getSamplesFromBytes(
                bytes,
                samples,
                this->mChannels,
                this->mFormat);
        }
        inline FMOD_RESULT getBytesFromSamples(uint32_t samples, uint32_t* bytes, bool roundup = true) {
            return this->getBytesFromSamples(
                samples,
                bytes,
                this->mChannels,
                this->mFormat,
                roundup);
        }

        inline FMOD_RESULT getBitsFromFormat(int32_t* bits) {
            return this->getBitsFromFormat(
                this->mFormat,
                bits);
        }

        static inline FMOD_RESULT getBitsFromFormat(FMOD_SOUND_FORMAT format, int32_t* bits) {
            switch (format) {
            case FMOD_SOUND_FORMAT_PCM8: {
                *bits = 8;
                break;
            }
            case FMOD_SOUND_FORMAT_PCM16: {
                *bits = 16;
                break;
            }
            case FMOD_SOUND_FORMAT_PCM24: {
                *bits = 24;
                break;
            }
            case FMOD_SOUND_FORMAT_PCM32: {
                *bits = 32;
                break;
            }
            case FMOD_SOUND_FORMAT_PCMFLOAT: {
                *bits = 32;
                break;
            }
            case FMOD_SOUND_FORMAT_GCADPCM: {
                *bits = 0;
                break;
            }
            case FMOD_SOUND_FORMAT_IMAADPCM: {
                *bits = 0;
                break;
            }
            case FMOD_SOUND_FORMAT_XMA: {
                *bits = 0;
                break;
            }
            case FMOD_SOUND_FORMAT_VAG: {
                *bits = 0;
                break;
            }
            case FMOD_SOUND_FORMAT_MPEG: {
                *bits = 0;
                break;
            }
            case FMOD_SOUND_FORMAT_NONE: {
                *bits = 0;
                break;
            }
            default:
                return FMOD_ERR_FORMAT;
            }
            return FMOD_OK;
        }

        static inline FMOD_RESULT getFormatFromBits(int32_t bits, FMOD_SOUND_FORMAT* format) {
            switch (bits) {
            case 8: {
                *format = FMOD_SOUND_FORMAT_PCM8;
                break;
            }
            case 16: {
                *format = FMOD_SOUND_FORMAT_PCM16;
                break;
            }
            case 24: {
                *format = FMOD_SOUND_FORMAT_PCM24;
                break;
            }
            case 32: {
                *format = FMOD_SOUND_FORMAT_PCM32;
                break;
            }
            default:
                return FMOD_ERR_FORMAT;
            }
            return FMOD_OK;
        }

        static inline FMOD_RESULT getSamplesFromBytes(uint32_t bytes, uint32_t* samples, int32_t channels, FMOD_SOUND_FORMAT format) {
            int32_t bits = 0;

            if (!channels) {
                return FMOD_ERR_INVALID_PARAM;
            }

            getBitsFromFormat(format, &bits);

            if (bits) {
                *samples = static_cast<uint32_t>(static_cast<FMOD_UINT64>(bytes) * FMOD_UINT64(8ULL) / static_cast<FMOD_UINT64>(bits));
            } else {
                switch (format) {
                case FMOD_SOUND_FORMAT_GCADPCM: {
                    *samples = bytes * 14 / 8;
                    break;
                }
                case FMOD_SOUND_FORMAT_IMAADPCM: {
                    *samples = bytes * 64 / 36;
                    break;
                }
                case FMOD_SOUND_FORMAT_VAG: {
                    *samples = bytes * 28 / 16;
                    break;
                }
                case FMOD_SOUND_FORMAT_XMA: {
                    *samples = bytes;
                    return FMOD_OK;
                }
                case FMOD_SOUND_FORMAT_MPEG: {
                    *samples = bytes;
                    return FMOD_OK;
                }
                case FMOD_SOUND_FORMAT_NONE: {
                    *samples = 0;
                    break;
                }
                default:
                    return FMOD_ERR_FORMAT;
                }
            }

            *samples /= channels;

            return FMOD_OK;
        }

        static inline FMOD_RESULT getBytesFromSamples(uint32_t samples, uint32_t* bytes, uint32_t channels, FMOD_SOUND_FORMAT format, bool roundup = true) {
            int32_t bits = 0;

            getBitsFromFormat(format, &bits);

            if (bits) {
                *bytes = static_cast<uint32_t>(static_cast<FMOD_UINT64>(samples) * static_cast<FMOD_UINT64>(bits) / 8);
            } else {
                switch (format) {
                case FMOD_SOUND_FORMAT_GCADPCM: {
                    *bytes = ((samples + (roundup ? 13 : 0)) / 14) * 8;
                    break;
                }
                case FMOD_SOUND_FORMAT_IMAADPCM: {
                    *bytes = ((samples + (roundup ? 63 : 0)) / 64) * 36;
                    break;
                }
                case FMOD_SOUND_FORMAT_VAG: {
                    *bytes = ((samples + (roundup ? 27 : 0)) / 28) * 16;
                    break;
                }
                case FMOD_SOUND_FORMAT_XMA: {
                    *bytes = samples;
                    return FMOD_OK;
                }
                case FMOD_SOUND_FORMAT_MPEG: {
                    *bytes = samples;
                    return FMOD_OK;
                }
                case FMOD_SOUND_FORMAT_NONE: {
                    *bytes = 0;
                    break;
                }
                default:
                    return FMOD_ERR_FORMAT;
                }
            }

            *bytes *= channels;

            return FMOD_OK;
        }

        static FMOD_RESULT validate(Sound* sound, SoundI** soundi);
};

} // namespace FMOD

#endif
