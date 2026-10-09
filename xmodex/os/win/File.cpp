#include "xmodex/os/File.hpp"
#include <windows.h>

FMOD_RESULT FMOD_OS_File_Cancel(void* handle) {
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Close(void* handle) {
    if (handle == INVALID_HANDLE_VALUE) {
        return FMOD_ERR_FILE_BAD;
    }

    CloseHandle(handle);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Open(const char* name, const char* mode, int32_t unicode, uint32_t* filesize, void** handle) {
    if (unicode) {
        *handle = CreateFileW(reinterpret_cast<LPCWSTR>(name), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    } else {
        *handle = CreateFileA(reinterpret_cast<LPCSTR>(name), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    if (*handle == INVALID_HANDLE_VALUE) {
        auto err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND && err != ERROR_INVALID_NAME) {
            return FMOD_ERR_FILE_BAD;
        }

        return FMOD_ERR_FILE_NOTFOUND;
    }

    *filesize = static_cast<uint32_t>(GetFileSize(*handle, nullptr));

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Read(void* handle, void* buf, uint32_t count, uint32_t* read) {
    DWORD rd;

    if (!ReadFile(handle, buf, count, &rd, nullptr)) {
        auto lasterr = GetLastError();
        return FMOD_ERR_FILE_BAD;
    }

    *read = rd;

    if (rd != count) {
        return FMOD_ERR_FILE_EOF;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Seek(void* handle, uint32_t offset) {
    auto newpos = SetFilePointer(handle, offset, nullptr, FILE_BEGIN);

    if (newpos == static_cast<DWORD>(-1)) {
        return FMOD_ERR_FILE_BAD;
    }

    return FMOD_OK;
}
