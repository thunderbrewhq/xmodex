#ifndef XMODEX_FILE_USER_FILE_HPP
#define XMODEX_FILE_USER_FILE_HPP

#include "xmodex/file/File.hpp"

namespace FMOD {

class UserFile : public File {
    private:
        FMOD_FILE_OPENCALLBACK mOpenCallback;
        FMOD_FILE_CLOSECALLBACK mCloseCallback;
        FMOD_FILE_READCALLBACK mReadCallback;
        FMOD_FILE_SEEKCALLBACK mSeekCallback;
        void* mHandle;
        void* mUserData;

    public:
        UserFile();

        FMOD_RESULT setUserCallbacks(FMOD_FILE_OPENCALLBACK open, FMOD_FILE_CLOSECALLBACK close, FMOD_FILE_READCALLBACK read, FMOD_FILE_SEEKCALLBACK seek);

        // virtual File methods
        FMOD_RESULT reallyOpen(const char* name_or_data, uint32_t* filesize);
        FMOD_RESULT reallyClose();
        FMOD_RESULT reallyRead(void* buffer, uint32_t size, uint32_t* rd);
        FMOD_RESULT reallySeek(uint32_t pos);

        static FMOD_FILE_OPENCALLBACK gOpenCallback;
        static FMOD_FILE_CLOSECALLBACK gCloseCallback;
        static FMOD_FILE_READCALLBACK gReadCallback;
        static FMOD_FILE_SEEKCALLBACK gSeekCallback;
};

} // namespace FMOD

#endif
