#ifndef XMODEX_FILE_MEMORY_FILE_HPP
#define XMODEX_FILE_MEMORY_FILE_HPP

#include "xmodex/file/File.hpp"

namespace FMOD {

class MemoryFile : public File {
    public:
        uint32_t mPosition;
        void* mMem;

    public:
        MemoryFile();

        // virtual File methods
        FMOD_RESULT reallyOpen(const char* name_or_data, uint32_t* filesize);
        FMOD_RESULT reallyClose();
        FMOD_RESULT reallyRead(void* buffer, uint32_t size, uint32_t* read);
        FMOD_RESULT reallySeek(uint32_t pos);
};

} // namespace FMOD

#endif
