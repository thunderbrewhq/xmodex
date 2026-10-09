#include "xmodex/os/CDDA.hpp"
#include "xmodex/String.hpp"

bool FMOD_OS_CDDA_IsDriveLetter(const char* name) {
    if (((name[1] == ':') && (name[2] == 0)) || ((name[2] == ':') && (name[4] == 0))) {
        if (FMOD_isalpha(name[0])) {
            return true;
        }
    }

    return false;
}

bool FMOD_OS_CDDA_IsDeviceName(const char* name) {
    if (!name) {
        return false;
    }

    if (FMOD_OS_CDDA_IsDriveLetter(name)) {
        return true;
    } else if ((name[2] == ':') && (name[5] == ':') && (name[8] == 0)) {
        if (FMOD_isdigit(name[0]) && FMOD_isdigit(name[1]) &&
            FMOD_isdigit(name[3]) && FMOD_isdigit(name[4]) &&
            FMOD_isdigit(name[6]) && FMOD_isdigit(name[7])) {
            return true;
        }
    }

    return false;
}
