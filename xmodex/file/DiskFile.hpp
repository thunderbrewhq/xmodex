#ifndef XMODEX_FILE_DISK_FILE_HPP
#define XMODEX_FILE_DISK_FILE_HPP

#include "xmodex/file/File.hpp"

namespace FMOD {

class DiskFile : public File {
    private:
        void* mHandle;

    public:
        DiskFile();

        // virtual File methods
        FMOD_RESULT reallyOpen(const char* name_or_data, uint32_t* filesize);
        FMOD_RESULT reallyClose();
        FMOD_RESULT reallyRead(void* buffer, uint32_t size, uint32_t* rd);
        FMOD_RESULT reallySeek(uint32_t pos);
        FMOD_RESULT reallyCancel();
};

} // namespace FMOD

#endif
