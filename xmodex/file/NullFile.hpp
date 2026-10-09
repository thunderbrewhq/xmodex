#ifndef XMODEX_FILE_NULL_FILE_HPP
#define XMODEX_FILE_NULL_FILE_HPP

#include "xmodex/file/File.hpp"

namespace FMOD {

class NullFile : public File {
    private:
        uint32_t mPosition;

    public:
        // virtual File methods
        FMOD_RESULT reallySeek(uint32_t pos);
        FMOD_RESULT reallyOpen(const char* name_or_data, uint32_t* filesize);
        FMOD_RESULT reallyClose();
        FMOD_RESULT reallyRead(void* buffer, uint32_t size, uint32_t* read);
};

} // namespace FMOD

#endif
