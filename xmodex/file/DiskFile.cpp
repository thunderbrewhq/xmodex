#include "xmodex/file/DiskFile.hpp"
#include "fmod.hpp"
#include "xmodex/os/File.hpp"
#include "xmodex/os/Thread.hpp"

namespace FMOD {

DiskFile::DiskFile() {
    this->mDeviceType = DEVICE_DISK;
}

FMOD_RESULT DiskFile::reallyCancel() {
    return FMOD_OS_File_Cancel(this->mHandle);
}

FMOD_RESULT DiskFile::reallyClose() {
    FMOD_RESULT result = FMOD_OK;

    if (this->mHandle) {
        result = FMOD_OS_File_Close(this->mHandle);
        this->mHandle = nullptr;
    }

    return result;
}

FMOD_RESULT DiskFile::reallyOpen(const char* name_or_data, uint32_t* filesize) {
    FMOD_RESULT result;
    char path[2048];

    if (this->mUnicode) {
        if (!FMOD_strlenW(reinterpret_cast<const uint16_t*>(name_or_data))) {
            return FMOD_ERR_FILE_NOTFOUND;
        }
    } else {
        if (!FMOD_strlen(name_or_data)) {
            return FMOD_ERR_FILE_NOTFOUND;
        }
    }

#if defined(XMODEX_SYSTEM_WIN)
    if (!_fullpath(path, name_or_data, 2047)) {
        return FMOD_ERR_INVALID_PARAM;
    }
#else
    FMOD_strcpy(path, name_or_data);
#endif

    result = this->setName(path);
    if (result != FMOD_OK) {
        return result;
    }

    result = FMOD_OS_File_Open(name_or_data, "rb", this->mUnicode, filesize, &this->mHandle);
    if (result != FMOD_OK) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT DiskFile::reallyRead(void* buffer, uint32_t size, uint32_t* rd) {
    FMOD_RESULT result;
    uint32_t read;
    FMOD_UINT_NATIVE threadid = 0;

    FMOD_OS_Thread_GetCurrentID(&threadid);

    if (this->mSystem && threadid != this->mSystem->mMainThreadID) {
        FMOD_File_SetDiskBusy(true);
    }

    result = FMOD_OS_File_Read(this->mHandle, buffer, size, &read);

    if (this->mSystem && threadid != this->mSystem->mMainThreadID) {
        FMOD_File_SetDiskBusy(false);
    }

    if (rd) {
        *rd = read;
    }

    if (result == FMOD_OK && size != read) {
        return FMOD_ERR_FILE_EOF;
    }

    return result;
}

FMOD_RESULT DiskFile::reallySeek(uint32_t pos) {
    return FMOD_OS_File_Seek(this->mHandle, pos);
}

} // namespace FMOD
