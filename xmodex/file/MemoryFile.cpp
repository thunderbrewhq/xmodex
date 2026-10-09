#include "xmodex/file/MemoryFile.hpp"

namespace FMOD {

MemoryFile::MemoryFile() {
    this->mDeviceType = DEVICE_MEMORY;
}

FMOD_RESULT MemoryFile::reallyOpen(const char* name_or_data, uint32_t* filesize) {
    this->mMem = const_cast<char*>(name_or_data);
    this->mPosition = 0;
    this->mBufferIsString = false;
    return FMOD_OK;
}

FMOD_RESULT MemoryFile::reallyClose() {
    return FMOD_OK;
}

FMOD_RESULT MemoryFile::reallyRead(void* buffer, uint32_t size, uint32_t* read) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mPosition + size > this->mLength) {
        size = this->mLength - this->mPosition;
        result = FMOD_ERR_FILE_EOF;
    }

    FMOD_memcpy(buffer, reinterpret_cast<char*>(this->mMem) + this->mPosition, size);

    *read = size;
    this->mPosition += size;

    return result;
}

FMOD_RESULT MemoryFile::reallySeek(uint32_t pos) {
    this->mPosition = pos;

    if (this->mPosition > this->mLength) {
        this->mPosition = this->mLength;
    }

    return FMOD_OK;
}

} // namespace FMOD
