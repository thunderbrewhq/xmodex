#include "xmodex/Thread.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/os/Thread.hpp"
#include "xmodex/os/Time.hpp"

namespace FMOD {

Thread::Thread() {
    this->mHandle = nullptr;
    this->mRunning = 0;
    this->mStack = nullptr;
    this->mUserData = nullptr;
    this->mSema = nullptr;
    this->mEndSema = nullptr;
    this->mPeriod = 0;
    this->mUserCallback = nullptr;
}

Thread::~Thread() {
}

FMOD_RESULT Thread::threadFunc() {
    return FMOD_OK;
}

FMOD_RESULT Thread::initThread(const char* name, void (*func)(void* userdata), void* userdata, PRIORITY priority, void* stack, int32_t stacksize, bool usesemaphore, int32_t sleepperiod) {
    FMOD_RESULT result;
    FMOD_THREAD_PRIORITY pri;

    this->mUserCallback = func;
    this->mUserData = userdata;
    this->mPeriod = sleepperiod;
    this->mRunning = 0;

    if (usesemaphore) {
        result = FMOD_OS_Semaphore_Create(&this->mSema);
        if (result != FMOD_OK) {
            return result;
        }
    }

    switch (priority) {
    case PRIORITY_VERYLOW: {
        pri = FMOD_THREAD_PRIORITY_VERYLOW;
        break;
    }
    case PRIORITY_LOW: {
        pri = FMOD_THREAD_PRIORITY_LOW;
        break;
    }
    case PRIORITY_NORMAL: {
        pri = FMOD_THREAD_PRIORITY_NORMAL;
        break;
    }
    case PRIORITY_HIGH: {
        pri = FMOD_THREAD_PRIORITY_HIGH;
        break;
    }
    case PRIORITY_VERYHIGH: {
        pri = FMOD_THREAD_PRIORITY_VERYHIGH;
        break;
    }
    case PRIORITY_CRITICAL: {
        pri = FMOD_THREAD_PRIORITY_CRITICAL;
        break;
    }
    default: {
        return FMOD_ERR_INVALID_PARAM;
    }
    }

    if (name) {
        FMOD_strncpy(this->mName, name, FMOD_STRING_MAXNAMELEN);
    } else {
        FMOD_strcpy(this->mName, "?????");
    }

    result = FMOD_OS_Thread_Create(name, callback, this, pri, stack, stacksize, &this->mHandle);
    if (result != FMOD_OK) {
        return result;
    }

    while (!this->mRunning) {
        FMOD_OS_Time_Sleep(1);
    }

    return FMOD_OK;
}

FMOD_RESULT Thread::closeThread() {
    FMOD_RESULT result;

    if (!this->mRunning) {
        return FMOD_OK;
    }

    result = FMOD_OS_Semaphore_Create(&this->mEndSema);
    if (result != FMOD_OK) {
        return result;
    }

    this->mRunning = false;

    if (this->mSema) {
        result = FMOD_OS_Semaphore_Signal(this->mSema);
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = FMOD_OS_Semaphore_Wait(this->mEndSema);
    if (result != FMOD_OK) {
        return result;
    }

    if (this->mSema) {
        result = FMOD_OS_Semaphore_Free(this->mSema);
        if (result != FMOD_OK) {
            return result;
        }
        this->mSema = nullptr;
    }

    result = FMOD_OS_Semaphore_Free(this->mEndSema);
    if (result != FMOD_OK) {
        return result;
    }
    this->mEndSema = nullptr;

    result = FMOD_OS_Thread_Destroy(this->mHandle);
    if (result != FMOD_OK) {
        return result;
    }
    this->mHandle = nullptr;

    if (this->mStack) {
        FMOD_Memory_Free(this->mStack);
        this->mStack = nullptr;
    }

    return FMOD_OK;
}

FMOD_RESULT Thread::getCurrentThreadID(FMOD_UINT_NATIVE* id) {
    FMOD_OS_Thread_GetCurrentID(id);

    return FMOD_OK;
}

FMOD_RESULT Thread::wakeupThread(bool frominterrupt) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mSema) {
        result = FMOD_OS_Semaphore_Signal(this->mSema, frominterrupt);
    }

    return result;
}

uintptr_t F_CALLBACK Thread::callback(void* data) {
    FMOD_UINT_NATIVE id;
    auto thread = reinterpret_cast<Thread*>(data);

    FMOD_OS_Thread_GetCurrentID(&id);
    gGlobal->gSystemPool->getCurrentThreadID(id);

    thread->mRunning = true;

    while (thread->mRunning) {
        if (thread->mSema) {
            FMOD_OS_Semaphore_Wait(thread->mSema);
        }
        if (thread->mRunning) {
            if (thread->mUserCallback) {
                thread->mUserCallback(thread->mUserData);
            } else {
                thread->threadFunc();
            }

            if (thread->mPeriod) {
                FMOD_OS_Time_Sleep(thread->mPeriod);
            }
        }
    }

    gGlobal->gSystemPool->clearThreadID(id);

    FMOD_OS_Semaphore_Signal(thread->mEndSema);

    return 0;
}

} // namespace FMOD
