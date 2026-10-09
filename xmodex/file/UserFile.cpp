#include "xmodex/file/UserFile.hpp"

namespace FMOD {

FMOD_FILE_OPENCALLBACK UserFile::gOpenCallback = nullptr;
FMOD_FILE_CLOSECALLBACK UserFile::gCloseCallback = nullptr;
FMOD_FILE_READCALLBACK UserFile::gReadCallback = nullptr;
FMOD_FILE_SEEKCALLBACK UserFile::gSeekCallback = nullptr;

UserFile::UserFile() {
    this->mHandle = nullptr;
    this->mUserData = nullptr;
    this->mOpenCallback = nullptr;
    this->mCloseCallback = nullptr;
    this->mReadCallback = nullptr;
    this->mSeekCallback = nullptr;
    this->mDeviceType = DEVICE_USER;
}

FMOD_RESULT UserFile::setUserCallbacks(FMOD_FILE_OPENCALLBACK open, FMOD_FILE_CLOSECALLBACK close, FMOD_FILE_READCALLBACK read, FMOD_FILE_SEEKCALLBACK seek) {
    if (!open || !close || !read || !seek) {
        open = nullptr;
        close = nullptr;
        read = nullptr;
        seek = nullptr;
    }

    this->mCloseCallback = close;
    this->mOpenCallback = open;
    this->mSeekCallback = seek;
    this->mReadCallback = read;
    return FMOD_OK;
}

FMOD_RESULT UserFile::reallyOpen(const char* name_or_data, uint32_t* filesize) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mOpenCallback) {
        result = this->mOpenCallback(name_or_data, this->mUnicode, filesize, &this->mHandle, &this->mUserData);
    } else if (gOpenCallback) {
        result = gOpenCallback(name_or_data, this->mUnicode, filesize, &this->mHandle, &this->mUserData);
    }

    return result;
}

FMOD_RESULT UserFile::reallyClose() {
    if (this->mCloseCallback) {
        this->mCloseCallback(this->mHandle, this->mUserData);
    } else if (gCloseCallback) {
        gCloseCallback(this->mHandle, this->mUserData);
    }

    return FMOD_OK;
}

FMOD_RESULT UserFile::reallyRead(void* buffer, uint32_t size, uint32_t* rd) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mReadCallback) {
        result = this->mReadCallback(this->mHandle, buffer, size, rd, this->mUserData);
    } else if (gReadCallback) {
        result = gReadCallback(this->mHandle, buffer, size, rd, this->mUserData);
    }

    return result;
}

FMOD_RESULT UserFile::reallySeek(uint32_t pos) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mSeekCallback) {
        result = this->mSeekCallback(this->mHandle, pos, this->mUserData);
    } else if (gSeekCallback) {
        result = gSeekCallback(this->mHandle, pos, this->mUserData);
    }

    return result;
}

} // namespace FMOD
