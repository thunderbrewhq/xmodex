#include "xmodex/channel/ChannelRealManual3D.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/SpeakerMap.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/sound/SoundI.hpp"

namespace FMOD {

ChannelRealManual3D::ChannelRealManual3D() {
    this->mAngleToListener = 0.0f;
}

FMOD_RESULT ChannelRealManual3D::set2DFreqVolumePanFor3D() {
    float lrpan[FMOD_CHANNEL_MAXINPUTCHANNELS] = { 0 };
    float fbpan[FMOD_CHANNEL_MAXINPUTCHANNELS] = { 0 };
    int32_t subchannel;
    int32_t numsubchannels;
    float lrpan_base = 0.0f;
    float fbpan_base = 0.0f;
    bool monospread = false;

    if (!(this->mMode & FMOD_3D)) {
        return FMOD_OK;
    }
    if (this->mSound) {
        if (this->mSound->mSubSampleParent) {
            numsubchannels = this->mSound->mSubSampleParent->mChannels;
        } else {
            numsubchannels = this->mSound->mChannels;
        }
    } else if (this->mDSP) {
        numsubchannels = this->mDSP->mDescription.channels;
    } else {
        return FMOD_ERR_INVALID_HANDLE;
    }

    if (this->mSystem->mNumListeners == 1) {
        FMOD_VECTOR currdiff;
        FMOD_VECTOR front, right;
        FMOD_VECTOR position;
        float distance;

        position = this->mParent->mPosition3D;

        if (this->mMode & FMOD_3D_HEADRELATIVE) {
            front.x = 0.0f;
            front.y = 0.0f;
            front.z = 1.0f;
            right.x = 1.0f;
            right.y = 0.0f;
            right.z = 0.0f;

            currdiff = position;
        } else {
            front = this->mSystem->mListener[0].mFront;
            right = this->mSystem->mListener[0].mRight;
            FMOD_Vector_Subtract(&position, &this->mSystem->mListener[0].mPosition, &currdiff);
        }

        if (this->mSystem->mFlags & FMOD_INIT_3D_RIGHTHANDED) {
            front.z = -front.z;
            currdiff.z = -currdiff.z;
        }

        // OFFSET: 0x94BEB2
        distance = FMOD_Vector_GetLength(&currdiff);

        if (distance <= 0) {
            currdiff.x = 0;
            currdiff.y = 0;
            currdiff.z = 0;
        } else {
            // OFFSET: 0x94BEDD
            currdiff.x /= static_cast<float>(distance);
            currdiff.y /= static_cast<float>(distance);
            currdiff.z /= static_cast<float>(distance);
        }

        // OFFSET: 0x94BEEB
        lrpan_base = FMOD_Vector_DotProduct(&currdiff, &right);
        fbpan_base = FMOD_Vector_DotProduct(&currdiff, &front);

        this->mAngleToListener = FMOD_Cart2Angle(static_cast<int32_t>(fbpan_base) * 256.0f, static_cast<int32_t>(lrpan_base) * 256.0f);

        float radian_spread = this->mParent->mSpread * (FMOD_PI / 180.0f);

        for (subchannel = 0; subchannel < numsubchannels; subchannel++) {
            if (this->mParent->mSpread == 0.0f || numsubchannels == 1) {
                lrpan[subchannel] = lrpan_base;
                fbpan[subchannel] = fbpan_base;
            } else {
                float spread_pos = 0.0f;
                switch (numsubchannels) {
                case 4:
                    switch (subchannel) {
                    case 0:
                        spread_pos = -1.0f;
                        break;
                    case 1:
                        spread_pos = 1.0f;
                        break;
                    case 2:
                        spread_pos = 0.0f;
                        break;
                    case 3:
                        spread_pos = 0.0f;
                        break;
                    }
                    break;

                case 6:
                    switch (subchannel) {
                    case 0:
                        spread_pos = -0.5f;
                        break;
                    case 1:
                        spread_pos = 0.5f;
                        break;
                    case 2:
                        spread_pos = 0.0f;
                        break;
                    case 3:
                        spread_pos = 0.0f;
                        break;
                    case 4:
                        spread_pos = -1.0f;
                        break;
                    case 5:
                        spread_pos = 1.0f;
                        break;
                    }
                    break;
                case 8:
                    switch (subchannel) {
                    case 0:
                        spread_pos = -0.333f;
                        break;
                    case 1:
                        spread_pos = 0.333f;
                        break;
                    case 2:
                        spread_pos = 0.0f;
                        break;
                    case 3:
                        spread_pos = 0.0f;
                        break;
                    case 4:
                        spread_pos = -1.0f;
                        break;
                    case 5:
                        spread_pos = 1.0f;
                        break;
                    case 6:
                        spread_pos = -0.667f;
                        break;
                    case 7:
                        spread_pos = 0.667f;
                        break;
                    }
                    break;
                default:
                    spread_pos = -1.0f + 2.0f * static_cast<float>(subchannel) / static_cast<float>(numsubchannels - 1);
                    break;
                }

                float theta = -0.5f * spread_pos * radian_spread;
                float cost = FMOD_COS(theta);
                float sint = FMOD_SIN(theta);

                lrpan[subchannel] = (cost * lrpan_base) - (sint * fbpan_base);
                fbpan[subchannel] = (cost * fbpan_base) + (sint * lrpan_base);
            }
        }
    }

    this->setFrequency(this->mParent->mFrequency);

    if (this->mSystem->mSpeakerMode != FMOD_SPEAKERMODE_MONO &&
        this->mSystem->mSpeakerMode != FMOD_SPEAKERMODE_RAW) {

        float speakerlevel[FMOD_SPEAKER_MAX][FMOD_CHANNEL_MAXINPUTCHANNELS];
        int32_t numspeakers = 0;
        int32_t numrealspeakers = 0;
        int32_t subchannel;

        // OFFSET: 0x94C1D4
        FMOD_memset(speakerlevel, 0, (sizeof(float) * FMOD_SPEAKER_MAX * FMOD_CHANNEL_MAXINPUTCHANNELS));

        for (subchannel = 0; subchannel < numsubchannels; subchannel++) {
            speakerlevel[FMOD_SPEAKER_LOW_FREQUENCY][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_LOW_FREQUENCY].mActive ? this->mParent->mSpeakerLFE : 0;
        }

        if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_STEREO) {
            numspeakers = 2;
            numrealspeakers = 2;
        } else if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_PROLOGIC) {
            numspeakers = 5;
            numrealspeakers = 6;
        } else if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_QUAD) {
            numspeakers = 4;
            numrealspeakers = 4;
        } else if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_SURROUND) {
            numspeakers = 5;
            numrealspeakers = 5;
        } else if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_5POINT1) {
            numspeakers = 5;
            numrealspeakers = 6;
        } else if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_7POINT1) {
            numspeakers = 7;
            numrealspeakers = 8;
        }

        if (this->mParent->m3DPanLevel > 0.0f) {
            for (subchannel = 0; subchannel < numsubchannels; subchannel++) {
                if (fbpan[subchannel] == 0 && lrpan[subchannel] == 0) {
                    // level = sqrt(2) / numspeakers
                    float level = 1.4142135f / numspeakers;

                    // OFFSET: 0x94C386
                    speakerlevel[FMOD_SPEAKER_FRONT_LEFT][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_FRONT_LEFT].mActive ? level : 0.0f;
                    speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_FRONT_RIGHT].mActive ? level : 0.0f;
                    speakerlevel[FMOD_SPEAKER_FRONT_CENTER][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_FRONT_CENTER].mActive ? level : 0.0f;
                    speakerlevel[FMOD_SPEAKER_BACK_LEFT][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_BACK_LEFT].mActive ? level : 0.0f;
                    speakerlevel[FMOD_SPEAKER_BACK_RIGHT][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_BACK_RIGHT].mActive ? level : 0.0f;
                    speakerlevel[FMOD_SPEAKER_SIDE_LEFT][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_SIDE_LEFT].mActive ? level : 0.0f;
                    speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][subchannel] = this->mSystem->mSpeaker[FMOD_SPEAKER_SIDE_RIGHT].mActive ? level : 0.0f;
                } else {
                    int32_t speaker;
                    int32_t subchannelangle;

                    if (lrpan[subchannel] == lrpan_base && fbpan[subchannel] == fbpan_base) {
                        subchannelangle = this->mAngleToListener;
                    } else {
                        subchannelangle = FMOD_Cart2Angle(fbpan[subchannel], lrpan[subchannel]);
                    }

                    if (!this->mSystem->mSpeakerList[0]) {

                    } else if (!this->mSystem->mSpeakerList[1]) {
                        speakerlevel[this->mSystem->mSpeakerList[0]->mSpeaker][subchannel] = 1.0f;
                    } else {
                        // offset: 0x94C56D
                        for (speaker = 1; speaker < numspeakers + 1; speaker++) {
                            FMOD_SPEAKERCONFIG* spkA;
                            FMOD_SPEAKERCONFIG* spkB;

                            spkA = this->mSystem->mSpeakerList[speaker - 1];
                            if (this->mSystem->mSpeakerList[speaker]) {
                                spkB = this->mSystem->mSpeakerList[speaker];
                            } else {
                                spkB = this->mSystem->mSpeakerList[0];
                            }

                            if (spkA->mXZAngle == spkB->mXZAngle) {
                                continue;
                            }

                            if ((spkA->mXZAngle <= subchannelangle && spkB->mXZAngle > subchannelangle) ||
                                (spkA->mXZAngle <= (subchannelangle + 360) && spkB->mXZAngle > (subchannelangle + 360))) {
                                float pan;

                                if (spkA->mXZAngle > subchannelangle) {
                                    pan = static_cast<float>((subchannelangle + 360) - spkA->mXZAngle) / static_cast<float>(spkB->mXZAngle - spkA->mXZAngle);
                                } else {
                                    pan = static_cast<float>(subchannelangle - spkA->mXZAngle) / static_cast<float>(spkB->mXZAngle - spkA->mXZAngle);
                                }

                                speakerlevel[spkA->mSpeaker][subchannel] = FMOD_SQRT(1.0f - pan);
                                speakerlevel[spkB->mSpeaker][subchannel] = FMOD_SQRT(pan);
                            }
                        }
                    }
                }
            }
        }

        if (numsubchannels > 1) {
            int32_t speaker;
            float levels[DSP_MAXLEVELS_OUT * DSP_MAXLEVELS_IN] = { 0 };
            int32_t numinputlevels;

            if (this->mParent->m3DPanLevel < 1.0f) {
                float _3dmix = this->mParent->m3DPanLevel;
                float _2dmix = 1.0f - this->mParent->m3DPanLevel;

                if (this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_PAN || this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERMIX) {
                    float fl = 0.0f;
                    float fr = 0.0f;
                    float c = 0.0f;
                    float lfe = 0.0f;
                    float bl = 0.0f;
                    float br = 0.0f;
                    float sl = 0.0f;
                    float sr = 0.0f;

                    if (this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_PAN) {
                        float pan = (this->mParent->mPan + 1.0f) / 2.0f;
                        float l;
                        float r;

                        if (pan <= 0.5f) {
                            l = 1.0f;
                            r = pan * 2.0f;
                        } else {
                            l = (1.0f - pan) * 2.0f;
                            r = 1.0f;
                        }

                        if (numsubchannels == 2 && !(this->mSound->mDefaultChannelMask & SPEAKER_ALLMONO)) {
                            fl = l;
                            fr = r;
                            c = 0;
                            lfe = 0;
                            bl = 0;
                            br = 0;
                            sl = 0;
                            sr = 0;
                        } else {
                            fl = l;
                            fr = r;
                            c = 1.0f;
                            lfe = 1.0f;
                            bl = l;
                            br = r;
                            sl = l;
                            sr = r;
                        }
                    } else if (this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERMIX) {
                        fl = this->mParent->mSpeakerFL;
                        fr = this->mParent->mSpeakerFR;
                        c = this->mParent->mSpeakerC;
                        lfe = this->mParent->mSpeakerLFE;
                        bl = this->mParent->mSpeakerBL;
                        br = this->mParent->mSpeakerBR;
                        sl = this->mParent->mSpeakerSL;
                        sr = this->mParent->mSpeakerSR;
                    }

                    FMOD_SPEAKERMAPTYPE speakermap = FMOD_SPEAKERMAPTYPE_DEFAULT;

                    DSPI::calculateSpeakerLevels(fl,
                                                 fr,
                                                 c,
                                                 lfe,
                                                 bl,
                                                 br,
                                                 sl,
                                                 sr,
                                                 this->mSystem->mSpeakerMode,
                                                 numsubchannels,
                                                 speakermap,
                                                 levels,
                                                 &numinputlevels);

                    for (subchannel = 0; subchannel < numsubchannels; subchannel++) {
                        speakerlevel[FMOD_SPEAKER_FRONT_LEFT][subchannel] = (speakerlevel[FMOD_SPEAKER_FRONT_LEFT][subchannel] * _3dmix) + (levels[(FMOD_SPEAKER_FRONT_LEFT * numinputlevels) + subchannel] * _2dmix);
                        speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][subchannel] = (speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][subchannel] * _3dmix) + (levels[(FMOD_SPEAKER_FRONT_RIGHT * numinputlevels) + subchannel] * _2dmix);
                        speakerlevel[FMOD_SPEAKER_FRONT_CENTER][subchannel] = (speakerlevel[FMOD_SPEAKER_FRONT_CENTER][subchannel] * _3dmix) + (levels[(FMOD_SPEAKER_FRONT_CENTER * numinputlevels) + subchannel] * _2dmix);
                        speakerlevel[FMOD_SPEAKER_BACK_LEFT][subchannel] = (speakerlevel[FMOD_SPEAKER_BACK_LEFT][subchannel] * _3dmix) + (levels[(FMOD_SPEAKER_BACK_LEFT * numinputlevels) + subchannel] * _2dmix);
                        speakerlevel[FMOD_SPEAKER_BACK_RIGHT][subchannel] = (speakerlevel[FMOD_SPEAKER_BACK_RIGHT][subchannel] * _3dmix) + (levels[(FMOD_SPEAKER_BACK_RIGHT * numinputlevels) + subchannel] * _2dmix);
                        speakerlevel[FMOD_SPEAKER_SIDE_LEFT][subchannel] = (speakerlevel[FMOD_SPEAKER_SIDE_LEFT][subchannel] * _3dmix) + (levels[(FMOD_SPEAKER_SIDE_LEFT * numinputlevels) + subchannel] * _2dmix);
                        speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][subchannel] = (speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][subchannel] * _3dmix) + (levels[(FMOD_SPEAKER_SIDE_RIGHT * numinputlevels) + subchannel] * _2dmix);
                    }
                } else if (this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERLEVELS && this->mParent->mLevels) {
                    numinputlevels = this->mSystem->mMaxInputChannels;

                    for (subchannel = 0; subchannel < numsubchannels; subchannel++) {
                        speakerlevel[FMOD_SPEAKER_FRONT_LEFT][subchannel] = (speakerlevel[FMOD_SPEAKER_FRONT_LEFT][subchannel] * _3dmix) + (this->mParent->mLevels[(FMOD_SPEAKER_FRONT_LEFT * numinputlevels) + subchannel] * _2dmix);
                        speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][subchannel] = (speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][subchannel] * _3dmix) + (this->mParent->mLevels[(FMOD_SPEAKER_FRONT_RIGHT * numinputlevels) + subchannel] * _2dmix);

                        if (numspeakers > 2) {
                            speakerlevel[FMOD_SPEAKER_FRONT_CENTER][subchannel] = (speakerlevel[FMOD_SPEAKER_FRONT_CENTER][subchannel] * _3dmix) + (this->mParent->mLevels[(FMOD_SPEAKER_FRONT_CENTER * numinputlevels) + subchannel] * _2dmix);

                            if (numspeakers > 3) {
                                speakerlevel[FMOD_SPEAKER_BACK_LEFT][subchannel] = (speakerlevel[FMOD_SPEAKER_BACK_LEFT][subchannel] * _3dmix) + (this->mParent->mLevels[(FMOD_SPEAKER_BACK_LEFT * numinputlevels) + subchannel] * _2dmix);
                                speakerlevel[FMOD_SPEAKER_BACK_RIGHT][subchannel] = (speakerlevel[FMOD_SPEAKER_BACK_RIGHT][subchannel] * _3dmix) + (this->mParent->mLevels[(FMOD_SPEAKER_BACK_RIGHT * numinputlevels) + subchannel] * _2dmix);

                                if (numspeakers > 5) {
                                    speakerlevel[FMOD_SPEAKER_SIDE_LEFT][subchannel] = (speakerlevel[FMOD_SPEAKER_SIDE_LEFT][subchannel] * _3dmix) + (this->mParent->mLevels[(FMOD_SPEAKER_SIDE_LEFT * numinputlevels) + subchannel] * _2dmix);
                                    speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][subchannel] = (speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][subchannel] * _3dmix) + (this->mParent->mLevels[(FMOD_SPEAKER_SIDE_RIGHT * numinputlevels) + subchannel] * _2dmix);
                                }
                            }
                        }
                    }
                }
            }

            if (this->mSystem->mSpeakerMode == FMOD_SPEAKERMODE_PROLOGIC && (this->mSound->mMode & FMOD_SOFTWARE)) /* Downmix logic. */
            {
                for (subchannel = 0; subchannel < numsubchannels; subchannel++) {
                    DSPI::calculateSpeakerLevels(speakerlevel[FMOD_SPEAKER_FRONT_LEFT][subchannel],
                                                 speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][subchannel],
                                                 speakerlevel[FMOD_SPEAKER_FRONT_CENTER][subchannel],
                                                 speakerlevel[FMOD_SPEAKER_LOW_FREQUENCY][subchannel],
                                                 speakerlevel[FMOD_SPEAKER_BACK_LEFT][subchannel],
                                                 speakerlevel[FMOD_SPEAKER_BACK_RIGHT][subchannel],
                                                 speakerlevel[FMOD_SPEAKER_SIDE_LEFT][subchannel],
                                                 speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][subchannel],
                                                 this->mSystem->mSpeakerMode,
                                                 1,
                                                 FMOD_SPEAKERMAPTYPE_DEFAULT,
                                                 levels,
                                                 &numinputlevels);

                    speakerlevel[FMOD_SPEAKER_FRONT_LEFT][subchannel] = levels[0];
                    speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][subchannel] = levels[1];
                    speakerlevel[FMOD_SPEAKER_FRONT_CENTER][subchannel] = 0;
                    speakerlevel[FMOD_SPEAKER_BACK_LEFT][subchannel] = 0;
                    speakerlevel[FMOD_SPEAKER_BACK_RIGHT][subchannel] = 0;
                    speakerlevel[FMOD_SPEAKER_SIDE_LEFT][subchannel] = 0;
                    speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][subchannel] = 0;
                }
            }

            if (monospread) {
                numsubchannels = 1;
            }

            for (speaker = 0; speaker < numrealspeakers; speaker++) {
                if (monospread) {
                    speakerlevel[speaker][0] = (speakerlevel[speaker][0] + speakerlevel[speaker][1]) / 2.0f;
                }
                this->setSpeakerLevels(speaker, speakerlevel[speaker], numsubchannels);
            }
        } else {
            if (this->mParent->m3DPanLevel < 1.0f) {
                float _3dmix = this->mParent->m3DPanLevel;
                float _2dmix = 1.0f - this->mParent->m3DPanLevel;

                if (this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_PAN) {
                    float pan = (this->mParent->mPan + 1.0f) / 2.0f;
                    float l = FMOD_SQRT(1.0f - pan);
                    float r = FMOD_SQRT(pan);

                    speakerlevel[FMOD_SPEAKER_FRONT_LEFT][0] = (speakerlevel[FMOD_SPEAKER_FRONT_LEFT][0] * _3dmix) + (l * _2dmix);
                    speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][0] * _3dmix) + (r * _2dmix);
                    speakerlevel[FMOD_SPEAKER_FRONT_CENTER][0] = (speakerlevel[FMOD_SPEAKER_FRONT_CENTER][0] * _3dmix);
                    speakerlevel[FMOD_SPEAKER_BACK_LEFT][0] = (speakerlevel[FMOD_SPEAKER_BACK_LEFT][0] * _3dmix);
                    speakerlevel[FMOD_SPEAKER_BACK_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_BACK_RIGHT][0] * _3dmix);
                    speakerlevel[FMOD_SPEAKER_SIDE_LEFT][0] = (speakerlevel[FMOD_SPEAKER_SIDE_LEFT][0] * _3dmix);
                    speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][0] * _3dmix);
                } else if (this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERMIX) {
                    speakerlevel[FMOD_SPEAKER_FRONT_LEFT][0] = (speakerlevel[FMOD_SPEAKER_FRONT_LEFT][0] * _3dmix) + (this->mParent->mSpeakerFL * _2dmix);
                    speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][0] * _3dmix) + (this->mParent->mSpeakerFR * _2dmix);
                    speakerlevel[FMOD_SPEAKER_FRONT_CENTER][0] = (speakerlevel[FMOD_SPEAKER_FRONT_CENTER][0] * _3dmix) + (this->mParent->mSpeakerC * _2dmix);
                    speakerlevel[FMOD_SPEAKER_BACK_LEFT][0] = (speakerlevel[FMOD_SPEAKER_BACK_LEFT][0] * _3dmix) + (this->mParent->mSpeakerBL * _2dmix);
                    speakerlevel[FMOD_SPEAKER_BACK_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_BACK_RIGHT][0] * _3dmix) + (this->mParent->mSpeakerBR * _2dmix);
                    speakerlevel[FMOD_SPEAKER_SIDE_LEFT][0] = (speakerlevel[FMOD_SPEAKER_SIDE_LEFT][0] * _3dmix) + (this->mParent->mSpeakerSL * _2dmix);
                    speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][0] * _3dmix) + (this->mParent->mSpeakerSR * _2dmix);
                } else if (this->mParent->mLastPanMode == FMOD_CHANNEL_PANMODE_SPEAKERLEVELS && this->mParent->mLevels) {
                    speakerlevel[FMOD_SPEAKER_FRONT_LEFT][0] = (speakerlevel[FMOD_SPEAKER_FRONT_LEFT][0] * _3dmix) + (this->mParent->mLevels[mSystem->mMaxInputChannels * FMOD_SPEAKER_FRONT_LEFT] * _2dmix);
                    speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][0] * _3dmix) + (this->mParent->mLevels[mSystem->mMaxInputChannels * FMOD_SPEAKER_FRONT_RIGHT] * _2dmix);

                    if (numspeakers > 2) {
                        speakerlevel[FMOD_SPEAKER_FRONT_CENTER][0] = (speakerlevel[FMOD_SPEAKER_FRONT_CENTER][0] * _3dmix) + (this->mParent->mLevels[mSystem->mMaxInputChannels * FMOD_SPEAKER_FRONT_CENTER] * _2dmix);

                        if (numspeakers > 3) {
                            speakerlevel[FMOD_SPEAKER_BACK_LEFT][0] = (speakerlevel[FMOD_SPEAKER_BACK_LEFT][0] * _3dmix) + (this->mParent->mLevels[mSystem->mMaxInputChannels * FMOD_SPEAKER_BACK_LEFT] * _2dmix);
                            speakerlevel[FMOD_SPEAKER_BACK_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_BACK_RIGHT][0] * _3dmix) + (this->mParent->mLevels[mSystem->mMaxInputChannels * FMOD_SPEAKER_BACK_RIGHT] * _2dmix);

                            if (numspeakers > 5) {
                                speakerlevel[FMOD_SPEAKER_SIDE_LEFT][0] = (speakerlevel[FMOD_SPEAKER_SIDE_LEFT][0] * _3dmix) + (this->mParent->mLevels[mSystem->mMaxInputChannels * FMOD_SPEAKER_SIDE_LEFT] * _2dmix);
                                speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][0] = (speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][0] * _3dmix) + (this->mParent->mLevels[mSystem->mMaxInputChannels * FMOD_SPEAKER_SIDE_RIGHT] * _2dmix);
                            }
                        }
                    }
                }
            }

            this->setSpeakerMix(speakerlevel[FMOD_SPEAKER_FRONT_LEFT][0],
                                speakerlevel[FMOD_SPEAKER_FRONT_RIGHT][0],
                                speakerlevel[FMOD_SPEAKER_FRONT_CENTER][0],
                                this->mParent->mSpeakerLFE,
                                speakerlevel[FMOD_SPEAKER_BACK_LEFT][0],
                                speakerlevel[FMOD_SPEAKER_BACK_RIGHT][0],
                                speakerlevel[FMOD_SPEAKER_SIDE_LEFT][0],
                                speakerlevel[FMOD_SPEAKER_SIDE_RIGHT][0]);
        }
    }

    this->setVolume(this->mParent->mVolume);

    return FMOD_OK;
}

} // namespace FMOD
