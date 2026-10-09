#include "xmodex/file/NullFile.hpp"

namespace FMOD {

FMOD_RESULT NullFile::reallyOpen(const char* name_or_data, uint32_t* filesize) {
    this->mPosition = 0;
    *filesize = 0;
    return FMOD_OK;
}

FMOD_RESULT NullFile::reallyClose() {
    return FMOD_OK;
}

FMOD_RESULT NullFile::reallyRead(void* buffer, uint32_t size, uint32_t* read) {
    FMOD_RESULT result = FMOD_OK;

    if (this->mPosition + size > this->mLength) {
        size = this->mLength - this->mPosition;
        result = FMOD_ERR_INVALID_PARAM;
    }

    *read = size;
    this->mPosition += size;

    return result;
}

FMOD_RESULT NullFile::reallySeek(uint32_t pos) {
    this->mPosition = pos;

    if (this->mPosition > this->mLength) {
        this->mPosition = this->mLength;
    }

    return FMOD_OK;
}

} // namespace FMOD
