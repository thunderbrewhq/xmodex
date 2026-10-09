#include "xmodex/os/File.hpp"
#include <cstdio>

FMOD_RESULT FMOD_OS_File_Cancel(void* handle) {
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Close(void* handle) {
    if (handle) {
        fclose(reinterpret_cast<FILE*>(handle));
    }
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Open(const char* name, const char* mode, int32_t unicode, uint32_t* filesize, void** handle) {
    // XXX: UTF-16 names aren't supported here
    auto file = fopen(name, mode);
    *handle = file;

    if (!file) {
        return FMOD_ERR_FILE_NOTFOUND;
    }

    if (filesize) {
        fseek(file, 0, SEEK_END);
        *filesize = ftell(file);
        fseek(file, 0, SEEK_SET);
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Read(void* handle, void* buf, uint32_t count, uint32_t* read) {
    if (!handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    auto file = reinterpret_cast<FILE*>(handle);

    *read = fread(buf, 1, count, file);

    if (feof(file)) {
        return FMOD_ERR_FILE_EOF;
    }

    if (ferror(file)) {
        return FMOD_ERR_FILE_BAD;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_File_Seek(void* handle, uint32_t offset) {
    if (!handle) {
        return FMOD_ERR_INVALID_PARAM;
    }

    if (fseek(reinterpret_cast<FILE*>(handle), offset, SEEK_SET) < 0) {
        return FMOD_ERR_FILE_BAD;
    }

    return FMOD_OK;
}
