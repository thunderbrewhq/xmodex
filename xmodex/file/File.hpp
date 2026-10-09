#ifndef XMODEX_FILE_FILE_HPP
#define XMODEX_FILE_FILE_HPP

#include "fmod.hpp"
#include "xmodex/LinkedListNode.hpp"
#include "xmodex/Metadata.hpp"
#include "xmodex/String.hpp"
#include "xmodex/SystemI.hpp"
#include "xmodex/file/FileThread.hpp"
#include "xmodex/os/Semaphore.hpp"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

namespace FMOD {

enum {
    DEVICE_USER = 0,
    DEVICE_MEMORY,
    DEVICE_RECORD,
    DEVICE_NET,
    DEVICE_CDDA,
    DEVICE_DISK
};

constexpr uint32_t FILE_SECTORSIZE = 0x800;

class File : public LinkedListNode {
        friend class Stream;
        friend class SystemI;
        friend class FileThread;
        friend class SoundI;
        friend class CodecWav;
        friend class CodecMPEG;

    protected:
        uint32_t mLength;
        uint32_t mLengthOriginal;
        uint32_t mFileSize;
        bool mBufferIsString;
        void* mRiderUserData;
        void* mRiderHandle;
        bool mUnicode;
        int32_t mDeviceType;

    public:
        char mName[FMOD_STRING_MAXNAMELEN];
        char mEncryptionKey[32];
        uint32_t mEncryptionKeyLength;
        int32_t mEncryptionKeyIndex;
        uint32_t mBufferPos;
        uint32_t mBufferSize;
        uint32_t mBufferSkip;
        uint32_t mBlockSize;
        uint32_t mBlockOffset;
        uint32_t mCurrentPosition;
        uint32_t mNextPosition;
        uint32_t mNextPositionDisplay;
        uint32_t mStartOffset;
        bool mBigEndian;
        bool mBusy;
        bool mStarving;
        bool mExit;
        int32_t mPercentBuffered;
        FMOD_RESULT mAsyncError;
        bool mFlipping;
        FileThread* mFileThread;
        FMOD_OS_SEMAPHORE* mSema;

        bool mSeekable;

    public:
        SystemI* mSystem;
        void* mBuffer;

    public:
        File();

        FMOD_RESULT init(uint32_t filesize, int32_t blocksize);
        FMOD_RESULT open(const char* name_or_data, uint32_t length, bool unicode, const char* encryptionkey = nullptr);
        bool readyForClose();
        FMOD_RESULT cancel();
        FMOD_RESULT flip(bool frommainthread);
        FMOD_RESULT seekAndReset();
        FMOD_RESULT checkBufferedStatus();
        FMOD_RESULT read(void* buffer, uint32_t size, uint32_t count, uint32_t* rd);
        FMOD_RESULT getByte(uint16_t* val);
        FMOD_RESULT getByte(uint32_t* val);
        FMOD_RESULT getByte(uint8_t* val);
        FMOD_RESULT getByte(int32_t* val);
        FMOD_RESULT getWord(uint16_t* val);
        FMOD_RESULT getWord(uint32_t* val);
        FMOD_RESULT getWord(int32_t* val);
        FMOD_RESULT getDword(uint32_t* val);
        FMOD_RESULT seek(int32_t pos, int32_t mode);
        FMOD_RESULT tell(uint32_t* pos);
        FMOD_RESULT setStartOffset(uint32_t offset);
        FMOD_RESULT getStartOffset(uint32_t* offset);
        FMOD_RESULT getName(char** name);
        FMOD_RESULT setName(char* name);
        FMOD_RESULT close();
        FMOD_RESULT getFileThread();
        FMOD_RESULT enableDoubleBuffer(uint32_t sizebytes);

        virtual FMOD_RESULT getMetadata(Metadata** metadata);
        virtual FMOD_RESULT getSize(uint32_t* size);
        virtual FMOD_RESULT reallyOpen(const char* name_or_data, uint32_t* filesize) = 0;
        virtual FMOD_RESULT reallyClose() = 0;
        virtual FMOD_RESULT reallyRead(void* buffer, uint32_t size, uint32_t* read) = 0;
        virtual FMOD_RESULT reallySeek(uint32_t pos) = 0;
        virtual FMOD_RESULT reallyCancel();

        static FMOD_RESULT shutDown();

        static FMOD_OS_CRITICALSECTION* gFileCrit;
        static LinkedListNode gFileThreadHead;
        static bool gUsesUserCallbacks;
};

} // namespace FMOD

#endif
