#include "xmodex/AsyncThread.hpp"
#include "xmodex/AsyncData.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/sound/SoundI.hpp"
#include "xmodex/sound/Stream.hpp"

namespace FMOD {

FMOD_OS_CRITICALSECTION* AsyncThread::gAsyncCrit = nullptr;
LinkedListNode AsyncThread::gAsyncHead;

AsyncThread::AsyncThread() {
    this->mCrit = nullptr;
    this->mThreadActive = false;
    this->mBusy = false;
    this->mDone = false;

    if (!gAsyncCrit) {
        FMOD_OS_CriticalSection_Create(&gAsyncCrit);
    }
}

void asyncThreadFunc(void* data) {
    reinterpret_cast<AsyncThread*>(data)->threadFunc();
}

FMOD_RESULT AsyncThread::init(bool owned) {
    FMOD_RESULT result;

    this->mOwned = owned;

    result = FMOD_OS_CriticalSection_Create(&this->mCrit);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mThread.initThread("FMOD thread for FMOD_NONBLOCKING", asyncThreadFunc, this, Thread::PRIORITY_HIGH, nullptr, 32768, true, 0);
    if (result != FMOD_OK) {
        return result;
    }

    this->mThreadActive = true;

    FMOD_OS_CriticalSection_Enter(gAsyncCrit);
    this->addBefore(&gAsyncHead);
    FMOD_OS_CriticalSection_Leave(gAsyncCrit);

    return FMOD_OK;
}

FMOD_RESULT AsyncThread::reallyRelease() {
    FMOD_OS_CriticalSection_Enter(this->mCrit);

    auto current = this->mCallbackHead.getNext();
    while (current != &this->mCallbackHead) {
        auto next = current->getNext();
        current->removeNode();
        FMOD_Memory_Free(current);
        current = next;
    }

    FMOD_OS_CriticalSection_Leave(this->mCrit);

    this->removeNode();

    this->mThreadActive = false;
    this->mThread.closeThread();

    if (this->mCrit) {
        FMOD_OS_CriticalSection_Free(this->mCrit);
    }

    FMOD_Memory_Free(this);
    return FMOD_OK;
}

FMOD_RESULT AsyncThread::threadFunc() {
    FMOD_RESULT result = FMOD_OK;
    LinkedListNode* current;
    SoundI* sound = nullptr;
    SystemI* system;

    if (this->mThreadActive) {
        FMOD_OS_CriticalSection_Enter(this->mCrit);

        current = this->mHead.getNext();
        if (current != &this->mHead) {
            sound = reinterpret_cast<SoundI*>(current->getData());
            current->removeNode();
            this->mBusy = true;
        }

        FMOD_OS_CriticalSection_Leave(this->mCrit);

        if (sound) {
            system = sound->mSystem;

            if (sound->mOpenState == FMOD_OPENSTATE_LOADING) {
                if (sound->mMode & (FMOD_OPENMEMORY | FMOD_OPENMEMORY_POINT)) {
                    result = system->createSoundInternal(
                        reinterpret_cast<char*>(sound->mAsyncData->mNameData),
                        sound->mMode,
                        sound->mAsyncData->mExInfoExists ? &sound->mAsyncData->mExInfo : nullptr,
                        true,
                        &sound);
                } else {
                    result = system->createSoundInternal(
                        sound->mAsyncData->mName,
                        sound->mMode,
                        sound->mAsyncData->mExInfoExists ? &sound->mAsyncData->mExInfo : nullptr,
                        true,
                        &sound);
                }
            }
        } else if (sound->mOpenState == FMOD_OPENSTATE_SEEKING) {
            auto stream = reinterpret_cast<Stream*>(sound);

            result = stream->updateSubSound(stream->mSubSoundIndex, false);
            if (result == FMOD_OK) {
                result = stream->setPosition(0, FMOD_TIMEUNIT_PCM);
                if (result == FMOD_OK) {
                    result = stream->flush();
                }
            }
        }

        sound->mAsyncData->mThread = nullptr;
        sound->mAsyncData->mResult = result;
        sound->mFlags |= FMOD_SOUND_FLAG_DONOTRELEASE;
        sound->mOpenState = result == FMOD_OK ? FMOD_OPENSTATE_READY : FMOD_OPENSTATE_ERROR;

        if (sound->mSubSoundParent) {
            sound->mSubSoundParent->mOpenState = sound->mOpenState;
        }

        this->mBusy = false;

        if (sound->mAsyncData->mExInfoExists) {
            if (sound->mAsyncData->mExInfo.nonblockcallback) {
                sound->mUserData = sound->mAsyncData->mExInfo.userdata;
                sound->mAsyncData->mExInfo.nonblockcallback(reinterpret_cast<FMOD_SOUND*>(sound), result);
            }
        }

        sound->mFlags &= ~FMOD_SOUND_FLAG_DONOTRELEASE;

        this->release();
    }

    FMOD_OS_CriticalSection_Enter(this->mCrit);
    auto node = this->mCallbackHead.getNext();
    FMOD_OS_CriticalSection_Leave(this->mCrit);

    while (node != &this->mCallbackHead) {
        auto callback = reinterpret_cast<FMOD_ASYNC_CALLBACK>(node->getData());

        result = callback();
        if (result != FMOD_OK) {
            return result;
        }

        FMOD_OS_CriticalSection_Enter(this->mCrit);
        node = node->getNext();
        FMOD_OS_CriticalSection_Leave(this->mCrit);
    }

    return FMOD_OK;
}

FMOD_RESULT AsyncThread::shutDown() {
    if (gAsyncCrit) {
        FMOD_OS_CriticalSection_Enter(gAsyncCrit);

        auto current = gAsyncHead.getNext();
        while (current != &gAsyncHead) {
            auto next = current->getNext();
            auto asyncthread = reinterpret_cast<AsyncThread*>(current);

            asyncthread->reallyRelease();

            current = next;
        }

        FMOD_OS_CriticalSection_Leave(gAsyncCrit);
        if (gAsyncCrit) {
            FMOD_OS_CriticalSection_Free(gAsyncCrit);
            gAsyncCrit = nullptr;
        }
    }

    return FMOD_OK;
}

FMOD_RESULT AsyncThread::getAsyncThread(SoundI* sound) {
    FMOD_RESULT result;
    AsyncThread* asyncthread;
    bool found = false;

    if (!gAsyncCrit) {
        result = FMOD_OS_CriticalSection_Create(&gAsyncCrit);
        if (result != FMOD_OK) {
            return result;
        }
    }

    FMOD_OS_CriticalSection_Enter(gAsyncCrit);
    auto current = gAsyncHead.getNext();
    if (current != &gAsyncHead) {
        found = true;

        asyncthread = reinterpret_cast<AsyncThread*>(current);

        FMOD_OS_CriticalSection_Enter(asyncthread->mCrit);
        FMOD_OS_CriticalSection_Leave(asyncthread->mCrit);
    }
    FMOD_OS_CriticalSection_Leave(gAsyncCrit);

    if (!found) {
        asyncthread = FMOD_Object_Alloc(AsyncThread);
        if (!asyncthread) {
            return FMOD_ERR_MEMORY;
        }

        result = asyncthread->init(false);
        if (result != FMOD_OK) {
            return result;
        }
    }

    if (sound) {
        sound->mAsyncData->mThread = asyncthread;
    }

    return FMOD_OK;
}

FMOD_RESULT AsyncThread::update() {
    if (gAsyncCrit) {
        FMOD_OS_CriticalSection_Enter(gAsyncCrit);

        auto current = gAsyncHead.getNext();
        while (current != &gAsyncHead) {
            auto next = current->getNext();
            auto asyncthread = reinterpret_cast<AsyncThread*>(current);

            if (asyncthread->mDone) {
                asyncthread->reallyRelease();
            }

            current = next;
        }

        FMOD_OS_CriticalSection_Leave(gAsyncCrit);
    }

    return FMOD_OK;
}

FMOD_RESULT AsyncThread::release() {
    if (this->mOwned) {
        this->mDone = true;
    }

    return FMOD_OK;
}

} // namespace FMOD
