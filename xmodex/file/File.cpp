#include "xmodex/file/File.hpp"
#include "fmod.hpp"
#include "xmodex/Math.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/os/CDDA.hpp"
#include "xmodex/os/Semaphore.hpp"

namespace FMOD {

FMOD_OS_CRITICALSECTION* File::gFileCrit = nullptr;

bool File::gUsesUserCallbacks = false;

LinkedListNode File::gFileThreadHead;

File::File() {
    this->init(0, FILE_SECTORSIZE);
}

FMOD_RESULT File::init(uint32_t filesize, int32_t blocksize) {
    this->mBlockSize = blocksize;
    this->mBufferIsString = true;
    this->mSeekable = true;
    this->mBusy = false;
    this->mFlipping = false;
    this->mFileThread = nullptr;
    this->mRiderHandle = nullptr;
    this->mRiderUserData = nullptr;
    this->mSystem = nullptr;
    this->mBigEndian = false;
    this->mStartOffset = 0;
    this->mCurrentPosition = 0;
    this->mNextPosition = 0;
    this->mNextPositionDisplay = 0;
    this->mBlockOffset = 0;
    this->mBuffer = nullptr;
    this->mBufferPos = 0;
    this->mBufferSkip = 0;
    this->mAsyncError = FMOD_OK;
    this->mExit = false;
    this->mStarving = false;
    this->mEncryptionKeyLength = 0;
    this->mEncryptionKeyIndex = 0;
    this->mSema = nullptr;
    this->mPercentBuffered = 0;
    this->mLengthOriginal = filesize;
    this->mLength = filesize;
    this->mFileSize = filesize;

    FMOD_memset(this->mName, 0, FMOD_STRING_MAXNAMELEN);

    return FMOD_OK;
}

FMOD_RESULT File::open(const char* name_or_data, uint32_t length, bool unicode, const char* encryptionkey) {
    FMOD_RESULT result;

    this->mUnicode = unicode;
    this->mBigEndian = false;
    this->mStartOffset = 0;
    this->mCurrentPosition = 0;
    this->mNextPosition = 0;
    this->mNextPositionDisplay = 0;
    this->mBlockOffset = 0;
    this->mBuffer = nullptr;
    this->mBufferPos = 0;
    this->mBufferSkip = 0;
    this->mAsyncError = FMOD_OK;
    this->mExit = false;
    this->mStarving = false;
    this->mEncryptionKeyLength = 0;
    this->mEncryptionKeyIndex = 0;
    this->mLength = length;
    this->mFileSize = length;

    if (encryptionkey) {
        this->mEncryptionKeyLength = FMOD_strlen(encryptionkey);
        if (this->mEncryptionKeyLength > 32) {
            this->mEncryptionKeyLength = 32;
        }
        FMOD_memset(this->mEncryptionKey, 0, 32);
        FMOD_strncpy(this->mEncryptionKey, encryptionkey, this->mEncryptionKeyLength);
    }

    this->mBufferSize = this->mBlockSize;
    if (this->mBufferSize) {
        this->mBuffer = FMOD_Memory_Calloc(this->mBufferSize);
        if (!this->mBuffer) {
            return FMOD_ERR_MEMORY;
        }
    }

    result = this->reallyOpen(name_or_data, &this->mFileSize);
    if (result != FMOD_OK) {
        if (this->mBuffer) {
            FMOD_Memory_Free(this->mBuffer);
            this->mBuffer = nullptr;
        }
        return result;
    }

    if (name_or_data && this->mBufferIsString) {
        FMOD_strncpy(this->mName, name_or_data, FMOD_STRING_MAXNAMELEN);
        this->mName[FMOD_STRING_MAXNAMELEN - 1] = 0;
    }

    if (this->mSystem && this->mSystem->mOpenRiderCallback) {
        this->mSystem->mOpenRiderCallback(name_or_data, unicode, &this->mFileSize, &this->mRiderHandle, &this->mRiderUserData);
    }

    if (!this->mLength) {
        this->mLength = this->mFileSize;
    }
    this->mLengthOriginal = this->mLength;
    return FMOD_OK;
}

bool File::readyForClose() {
    return !this->mSema || !this->mBusy;
}

FMOD_RESULT File::cancel() {
    this->mExit = true;
    return this->reallyCancel();
}

FMOD_RESULT File::flip(bool frommainthread) {
    FMOD_RESULT result = FMOD_OK;

    if (frommainthread && this->mSema) {
        FMOD_OS_Semaphore_Wait(this->mSema);
    }

    uint32_t bytestoread = this->mBlockSize - this->mBufferSkip;
    uint32_t bytestoreadorig;
    uint32_t bytesread = 0;
    auto destptr = reinterpret_cast<char*>(this->mBuffer) + this->mBlockOffset + this->mBufferSkip;

    this->mBusy = true;

    bytestoreadorig = bytestoread;

    while (bytestoread) {
        uint32_t read;
        uint32_t r = 0;

        read = bytestoread;

        result = this->reallyRead(destptr, read, &r);
        if (result != FMOD_OK) {
            bytestoread = r;

            if (this->mLength == static_cast<uint32_t>(-1) && result == FMOD_ERR_FILE_EOF) {
                if (this->mNextPositionDisplay) {
                    this->mLengthOriginal = this->mNextPositionDisplay;
                    this->mFileSize = this->mNextPositionDisplay;
                    this->mLength = this->mNextPositionDisplay;
                }
            }
        }

        if (this->mSystem && this->mSystem->mReadRiderCallback) {
            this->mSystem->mReadRiderCallback(this->mRiderHandle, destptr, r, nullptr, this->mRiderUserData);
        }

        if (!r || r > bytestoread) {
            break;
        }

        bytestoread -= r;
        destptr += r;
        bytesread += r;
        this->mNextPositionDisplay += r;

        this->mPercentBuffered = static_cast<int32_t>((static_cast<float>(this->mNextPositionDisplay) - static_cast<float>(this->mCurrentPosition)) / static_cast<float>(this->mBufferSize) * 100.0f);
        if (this->mPercentBuffered < 0) {
            this->mPercentBuffered = 0;
        }

        if (this->mExit) {
            break;
        }
    }

    this->mBlockOffset += this->mBlockSize;
    if (this->mBlockOffset >= this->mBufferSize) {
        this->mBlockOffset = 0;
    }

    this->mBufferSkip = 0;
    this->mAsyncError = result;
    this->mFlipping = false;
    this->mBusy = false;

    if (this->mSema) {
        FMOD_OS_Semaphore_Signal(this->mSema);
    }

    return result;
}

FMOD_RESULT File::seekAndReset() {
    FMOD_RESULT result;
    uint32_t alignedpos;

    if (this->mSema && this->mBusy) {
        FMOD_OS_Semaphore_Wait(this->mSema);
        FMOD_OS_Semaphore_Signal(this->mSema);
    }

    alignedpos = this->mCurrentPosition;
    alignedpos /= this->mBufferSize;
    alignedpos *= this->mBufferSize;

    this->mBufferPos = this->mCurrentPosition - alignedpos;
    this->mBlockOffset = 0;
    this->mBufferSkip = 0;
    this->mNextPosition = alignedpos;
    this->mNextPositionDisplay = alignedpos;

    result = this->reallySeek(alignedpos);
    if (this->mSystem && this->mSystem->mSeekRiderCallback) {
        this->mSystem->mSeekRiderCallback(this->mRiderHandle, alignedpos, this->mRiderUserData);
    }
    return result;
}

FMOD_RESULT File::checkBufferedStatus() {
    FMOD_RESULT result = this->mAsyncError;
    int32_t diff;

    if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
        return result;
    }

    if (this->mNextPosition < this->mCurrentPosition) {
        diff = -1;
    } else {
        this->mPercentBuffered = static_cast<int32_t>((static_cast<float>(this->mNextPositionDisplay) - static_cast<float>(this->mCurrentPosition)) / static_cast<float>(this->mBufferSize) * 100.0f);
        if (this->mPercentBuffered < 0 || this->mBufferSkip) {
            this->mPercentBuffered = 0;
        }

        diff = this->mNextPosition - this->mCurrentPosition;
        diff += (this->mBlockSize - 1);
        diff /= this->mBlockSize;
    }

    if (this->mBufferSkip) {
        if (diff > 2) {
            this->mBufferSkip = 0;
        } else {
            diff = -1;
            this->mNextPositionDisplay = this->mBufferSkip;
            this->mNextPosition = this->mCurrentPosition - this->mBufferPos;
        }
    }

    if (diff != 2) {
        this->mStarving = true;
        if (this->mBusy) {
            FMOD_OS_Semaphore_Wait(this->mSema);
            FMOD_OS_Semaphore_Signal(this->mSema);
        }
        this->mStarving = false;
    }

    if (diff == 1 && this->mBufferSize > this->mBlockSize) {
        this->mBusy = true;
        this->mFlipping = true;

        FMOD_OS_Semaphore_Wait(this->mSema);
        this->mFileThread->mThread.wakeupThread();

        this->mNextPositionDisplay = this->mNextPosition;
        this->mNextPosition += this->mBlockSize;

        return FMOD_OK;
    } else if ((this->mBufferSize > this->mBlockSize && diff == 2) || (this->mBufferSize == this->mBlockSize && diff == 1)) {
        return FMOD_OK;
    } else if (diff && this->mSeekable) {
        result = this->seekAndReset();
        if (result != FMOD_OK) {
            return result;
        }
    }

    result = this->flip(true);
    if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
        return result;
    }

    if (this->mBufferSize == this->mBlockSize && result == FMOD_ERR_FILE_EOF && this->mLength == static_cast<uint32_t>(-1)) {
        return result;
    }

    this->mNextPositionDisplay = this->mNextPosition;
    this->mNextPosition += this->mBlockSize;

    if (this->mBufferPos >= this->mBlockSize) {
        result = this->flip(true);
        if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
            return result;
        }

        this->mNextPositionDisplay = this->mNextPosition;
        this->mNextPosition += this->mBlockSize;
    }

    return result;
}

FMOD_RESULT File::read(void* buffer, uint32_t size, uint32_t count, uint32_t* rd) {
    FMOD_RESULT result = FMOD_OK;
    auto oldsize = size;
    bool eof = false;
    uint32_t bytesleft;
    uint32_t r = 0;

    if (!buffer) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (static_cast<int32_t>(size * count) < 0) {
        return FMOD_ERR_INVALID_PARAM;
    }
    size *= count;

    if ((this->mCurrentPosition + size) > this->mStartOffset + this->mLength) {
        if (this->mCurrentPosition > this->mStartOffset + this->mLength) {
            return FMOD_ERR_FILE_BAD;
        }
        size = (this->mStartOffset + this->mLength) - this->mCurrentPosition;
        eof = true;
    }

    bytesleft = size;

    while (bytesleft) {
        size = bytesleft;

        if (this->mBlockSize == this->mBufferSize && size > this->mBlockSize && !this->mBufferPos && this->mSeekable) {
            if (this->mBlockSize) {
                if (this->mCurrentPosition != this->mNextPosition) {
                    result = this->seekAndReset();
                    if (result != FMOD_OK) {
                        return result;
                    }
                }

                size /= this->mBlockSize;
                size *= this->mBlockSize;
            }

            result = this->reallyRead(reinterpret_cast<char*>(buffer) + r, size, &size);

            this->mBusy = false;

            if (this->mSystem && this->mSystem->mReadRiderCallback) {
                this->mSystem->mReadRiderCallback(this->mRiderHandle, reinterpret_cast<char*>(buffer) + r, size, nullptr, this->mRiderUserData);
            }

            if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
                return result;
            }

            this->mNextPositionDisplay = this->mNextPosition;
            this->mNextPosition += size;

            if (!size) {
                result = FMOD_ERR_FILE_EOF;
            }

            if (result == FMOD_ERR_FILE_EOF) {
                break;
            }
        } else {
            result = this->checkBufferedStatus();

            if (result == FMOD_ERR_FILE_EOF && !(this->mBlockSize == this->mBufferSize && this->mLength == static_cast<uint32_t>(-1))) {
                result = FMOD_OK;
            }
            if (result != FMOD_OK) {
                break;
            }

            if (size > (this->mBlockSize - (this->mBufferPos % this->mBlockSize))) {
                size = this->mBlockSize - (this->mBufferPos % this->mBlockSize);
            }

            FMOD_memcpy(reinterpret_cast<char*>(buffer) + r, reinterpret_cast<char*>(this->mBuffer) + this->mBufferPos, size);
            this->mBufferPos += size;
            if (this->mBufferPos >= this->mBufferSize) {
                this->mBufferPos = 0;
            }
        }

        this->mCurrentPosition += size;
        r += size;
        bytesleft -= size;
    }

    if (oldsize == 2) {
        auto wptr = reinterpret_cast<uint16_t*>(buffer);

#if defined(XMODEX_CPU_LITTLEENDIAN)
        if (this->mBigEndian)
#elif defined(XMODEX_CPU_BIGENDIAN)
        if (!this->mBigEndian)
#endif
        {
            for (int32_t i = 0; i < (r / oldsize); i++) {
                wptr[i] = FMOD_SWAPENDIAN_WORD(wptr[i]);
            }
        }
    } else if (oldsize == 4) {
        auto dptr = reinterpret_cast<uint32_t*>(buffer);

#if defined(XMODEX_CPU_LITTLEENDIAN)
        if (this->mBigEndian)
#elif defined(XMODEX_CPU_BIGENDIAN)
        if (!this->mBigEndian)
#endif
        {
            for (int32_t i = 0; i < (r / oldsize); i++) {
                dptr[i] = FMOD_SWAPENDIAN_DWORD(dptr[i]);
            }
        }
    }

    r /= oldsize;

    if (this->mEncryptionKeyLength) {
        auto val = reinterpret_cast<uint8_t*>(buffer);
        for (int32_t i = 0; i < r; i++) {
            FMOD_DECRYPT(val[count], this->mEncryptionKey[this->mEncryptionKeyIndex]);
            this->mEncryptionKeyIndex++;
            if (this->mEncryptionKeyIndex >= this->mEncryptionKeyLength) {
                this->mEncryptionKeyIndex = 0;
            }
        }
    }

    if (rd) {
        *rd = r;
    }

    if (result == FMOD_OK && eof) {
        result = FMOD_ERR_FILE_EOF;
    }

    return result;
}

FMOD_RESULT File::getByte(uint16_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    uint8_t temp;

    result = this->read(&temp, 1, 1, &rd);

    if (val) {
        *val = static_cast<uint16_t>(temp);
    }

    return result;
}

FMOD_RESULT File::getByte(uint32_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    uint8_t temp;

    result = this->read(&temp, 1, 1, &rd);

    if (val) {
        *val = static_cast<uint32_t>(temp);
    }

    return result;
}

FMOD_RESULT File::getByte(uint8_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    uint8_t temp;

    result = this->read(&temp, 1, 1, &rd);

    if (val) {
        *val = temp;
    }

    return result;
}

FMOD_RESULT File::getByte(int32_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    int8_t temp;

    result = this->read(&temp, 1, 1, &rd);

    if (val) {
        *val = static_cast<int32_t>(temp);
    }

    return result;
}

FMOD_RESULT File::getWord(uint16_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    uint16_t temp;

    result = this->read(&temp, 2, 1, &rd);

    if (val) {
        *val = temp;
    }

    return result;
}

FMOD_RESULT File::getWord(uint32_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    uint16_t temp;

    result = this->read(&temp, 2, 1, &rd);

    if (val) {
        *val = static_cast<uint32_t>(temp);
    }

    return result;
}

FMOD_RESULT File::getWord(int32_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    int16_t temp;

    result = this->read(&temp, 2, 1, &rd);

    if (val) {
        *val = static_cast<int32_t>(temp);
    }

    return result;
}

FMOD_RESULT File::getDword(uint32_t* val) {
    FMOD_RESULT result;
    uint32_t rd;
    uint32_t temp;

    result = this->read(&temp, 4, 1, &rd);

    if (val) {
        *val = temp;
    }

    return result;
}

FMOD_RESULT File::seek(int32_t pos, int32_t mode) {
    FMOD_RESULT result = FMOD_OK;
    uint32_t newpos;

    if (mode != SEEK_SET && mode != SEEK_CUR && mode != SEEK_END) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (mode == SEEK_SET) {
        newpos = this->mStartOffset + pos;
    } else if (mode == SEEK_CUR) {
        newpos = this->mCurrentPosition + pos;
    } else if (mode == SEEK_END) {
        newpos = this->mStartOffset + this->mLength + pos;
    }

    if (newpos > this->mStartOffset + this->mLength) {
        if (pos >= 0) {
            newpos = this->mStartOffset + this->mLength;
        } else {
            newpos = 0;
        }
    }

    if (!this->mNextPosition && !this->mSeekable && newpos >= this->mBlockSize) {
        return FMOD_ERR_FILE_COULDNOTSEEK;
    }

    if ((this->mBlockSize == this->mBufferSize) && !this->mSeekable && (this->mNextPosition >= this->mBlockSize)) {
        if (this->mNextPosition && newpos < this->mNextPosition - this->mBlockSize) {
            return FMOD_ERR_FILE_COULDNOTSEEK;
        }
        if (newpos >= this->mNextPosition + this->mBlockSize) {
            return FMOD_ERR_FILE_COULDNOTSEEK;
        }
    }

    this->mCurrentPosition = newpos;

    if (this->mEncryptionKeyLength) {
        this->mEncryptionKeyIndex = this->mCurrentPosition % this->mEncryptionKeyLength;
    }

    if (this->mBufferSize) {
        this->mBufferPos = this->mCurrentPosition % this->mBufferSize;
    } else {
        result = this->reallySeek(newpos);

        if (this->mSystem && this->mSystem->mSeekRiderCallback) {
            this->mSystem->mSeekRiderCallback(this->mRiderHandle, newpos, this->mRiderUserData);
        }
    }

    return result;
}

FMOD_RESULT File::tell(uint32_t* pos) {
    if (!pos) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *pos = this->mCurrentPosition;
    *pos -= this->mStartOffset;

    return FMOD_OK;
}

FMOD_RESULT File::setStartOffset(uint32_t offset) {
    this->mStartOffset = offset;
    this->mLength = this->mLengthOriginal;

    if (this->mStartOffset + this->mLength > this->mFileSize) {
        this->mLength = this->mFileSize - this->mStartOffset;
    }

    return FMOD_OK;
}

FMOD_RESULT File::getStartOffset(uint32_t* offset) {
    if (!offset) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *offset = this->mStartOffset;

    return FMOD_OK;
}

FMOD_RESULT File::getName(char** name) {
    if (name) {
        *name = this->mName;
    }

    return FMOD_OK;
}

FMOD_RESULT File::setName(char* name) {
    if (!name) {
        return FMOD_ERR_INVALID_PARAM;
    }

    FMOD_strncpy(this->mName, name, FMOD_STRING_MAXNAMELEN);
    this->mName[FMOD_STRING_MAXNAMELEN - 1] = 0;

    return FMOD_OK;
}

FMOD_RESULT File::close() {
    FMOD_RESULT result;

    this->mExit = true;

    this->reallyCancel();

    if (this->mSema && this->mBusy) {
        FMOD_OS_Semaphore_Wait(this->mSema);
        FMOD_OS_Semaphore_Signal(this->mSema);
    }

    if (this->mFileThread) {
        FMOD_OS_CriticalSection_Enter(this->mFileThread->mFileListCrit);
        if (this == this->mFileThread->mFileListNext) {
            this->mFileThread->mFileListNext = this->getNext();
        }
        this->removeNode();
        FMOD_OS_CriticalSection_Leave(this->mFileThread->mFileListCrit);

        if (this->mFileThread->mOwned) {
            this->mFileThread->release();
        }

        this->mFileThread = nullptr;
    }

    result = this->reallyClose();
    if (this->mSema) {
        FMOD_OS_Semaphore_Free(this->mSema);
    }

    if (this->mSystem && this->mSystem->mCloseRiderCallback) {
        this->mSystem->mCloseRiderCallback(this->mRiderHandle, this->mRiderUserData);
    }

    if (this->mBuffer) {
        FMOD_Memory_Free(this->mBuffer);
        this->mBuffer = nullptr;
    }

    return result;
}

FMOD_RESULT File::getFileThread() {
    FMOD_RESULT result;
    int32_t devicetype;
    bool owned;
    bool found;
    FileThread* filethread = nullptr;

    devicetype = DEVICE_DISK;

    if (!FMOD_strnicmp("http://", this->mName, 7)) {
        devicetype = DEVICE_NET;
        owned = true;
    }

    if (FMOD_OS_CDDA_IsDeviceName(this->mName)) {
        devicetype = DEVICE_CDDA;
        owned = true;
    }

    found = false;
    if (devicetype == DEVICE_DISK) {
        auto current = gFileThreadHead.getNext();
        while (current != &gFileThreadHead) {
            filethread = reinterpret_cast<FileThread*>(current);
            if (filethread->mDeviceType == DEVICE_DISK) {
                found = true;
                break;
            }

            current = current->getNext();
        }
    }

    if (!found) {
        filethread = FMOD_Object_Alloc(FileThread);
        if (!filethread) {
            return FMOD_ERR_MEMORY;
        }

        result = filethread->init(devicetype, owned);
        if (result != FMOD_OK) {
            FMOD_Memory_Free(filethread);
            return result;
        }
    }

    this->mFileThread = filethread;
    return FMOD_OK;
}

FMOD_RESULT File::enableDoubleBuffer(uint32_t sizebytes) {
    FMOD_RESULT result;
    uint32_t oldblocksize;
    uint32_t oldbuffersize;

    if (!this->mBlockSize) {
        return FMOD_OK;
    }

    result = FMOD_OS_Semaphore_Create(&this->mSema);
    if (result != FMOD_OK) {
        return result;
    }

    FMOD_OS_Semaphore_Signal(this->mSema);

    if (sizebytes < FILE_SECTORSIZE) {
        sizebytes = FILE_SECTORSIZE;
    }
    if (sizebytes < this->mBlockSize) {
        sizebytes = this->mBlockSize;
    }

    oldblocksize = this->mBlockSize;
    oldbuffersize = this->mBufferSize;

    this->mBlockSize = sizebytes;
    this->mBlockSize /= oldblocksize;
    this->mBlockSize *= oldblocksize;

    this->mBufferSkip = oldblocksize;
    this->mBlockOffset = 0;
    this->mNextPosition = this->mBlockSize;
    this->mNextPositionDisplay = this->mNextPosition;

    this->mBufferSize = this->mBlockSize;
    this->mBufferSize *= 2;

    this->mBuffer = FMOD_Memory_ReAlloc(this->mBuffer, this->mBufferSize);
    if (!this->mBuffer) {
        return FMOD_ERR_MEMORY;
    }

    result = this->getFileThread();
    if (result != FMOD_OK) {
        return result;
    }

    FMOD_OS_CriticalSection_Enter(this->mFileThread->mFileListCrit);
    this->addAfter(&this->mFileThread->mFileListHead);
    FMOD_OS_CriticalSection_Leave(this->mFileThread->mFileListCrit);

    result = this->checkBufferedStatus();
    if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) {
        return result;
    }

    return FMOD_OK;
}

FMOD_RESULT File::getMetadata(Metadata** metadata) {
    return FMOD_ERR_TAGNOTFOUND;
}

FMOD_RESULT File::getSize(uint32_t* size) {
    *size = this->mLength;
    return FMOD_OK;
}

FMOD_RESULT File::reallyCancel() {
    return FMOD_OK;
}

FMOD_RESULT File::shutDown() {
    auto current = gFileThreadHead.getNext();
    while (current != &gFileThreadHead) {
        auto next = current->getNext();
        auto filethread = static_cast<FileThread*>(current);
        filethread->release();
        current = next;
    }

    // NetFile::shutDown();
    // CddaFile::shutDown();

    if (gFileCrit) {
        FMOD_OS_CriticalSection_Free(gFileCrit);
        gFileCrit = nullptr;
    }

    return FMOD_OK;
}

} // namespace FMOD

FMOD_RESULT F_API FMOD_File_SetDiskBusy(int32_t busy) {
    if (busy) {
        FMOD_OS_CriticalSection_Enter(FMOD::File::gFileCrit);
        FMOD::gGlobal->gFileBusy = true;
    } else {
        FMOD::gGlobal->gFileBusy = false;
        FMOD_OS_CriticalSection_Leave(FMOD::File::gFileCrit);
    }

    return FMOD_OK;
}

FMOD_RESULT F_API FMOD_File_GetDiskBusy(int32_t* busy) {
    if (!busy) {
        return FMOD_ERR_INVALID_PARAM;
    }

    *busy = FMOD::gGlobal->gFileBusy;

    return FMOD_OK;
}
