#include "xmodex/file/FileThread.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/file/File.hpp"

namespace FMOD {

FileThread::FileThread() {
    this->mFileListCrit = nullptr;
    this->mFileListCurrent = nullptr;
    this->mFileListNext = nullptr;
    this->mThreadActive = false;
    this->mDeviceType = 0;
}

void fileThreadFunc(void* data) {
    reinterpret_cast<FileThread*>(data)->threadFunc();
}

FMOD_RESULT FileThread::threadFunc() {
    if (this->mThreadActive) {
        FMOD_OS_CriticalSection_Enter(this->mFileListCrit);
        this->mFileListCurrent = this->mFileListHead.getNext();

        while (this->mFileListCurrent != &this->mFileListHead) {
            this->mFileListNext = this->mFileListCurrent->getNext();

            auto file = static_cast<File*>(this->mFileListCurrent);

            if (file->mFlipping) {
                FMOD_OS_CriticalSection_Leave(this->mFileListCrit);
                file->flip(false);
                FMOD_OS_CriticalSection_Enter(this->mFileListCrit);
            }

            this->mFileListCurrent = this->mFileListNext;
        }

        FMOD_OS_CriticalSection_Leave(this->mFileListCrit);
    }

    return FMOD_OK;
}

FMOD_RESULT FileThread::init(int32_t devicetype, bool owned) {
    FMOD_RESULT result;

    this->mDeviceType = devicetype;
    this->mOwned = owned;

    result = FMOD_OS_CriticalSection_Create(&this->mFileListCrit);
    if (result != FMOD_OK) {
        return result;
    }

    result = this->mThread.initThread("FMOD file thread", fileThreadFunc, this, Thread::PRIORITY_HIGH, nullptr, 8192, true, 0);
    if (result != FMOD_OK) {
        return result;
    }

    this->mThreadActive = true;
    this->addAfter(&File::gFileThreadHead);

    return FMOD_OK;
}

FMOD_RESULT FileThread::release() {
    this->removeNode();

    this->mThreadActive = false;

    this->mThread.closeThread();

    if (this->mFileListCrit) {
        FMOD_OS_CriticalSection_Free(this->mFileListCrit);
    }

    FMOD_Memory_Free(this);

    return FMOD_OK;
}

} // namespace FMOD
