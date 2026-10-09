#include "xmodex/os/Driver.hpp"
#include <windows.h>
#include <mmeapi.h>

FMOD_RESULT FMOD_OS_CheckDriverList(bool* devicelistchanged) {
    static int32_t lastNumDrivers = -1;

    int32_t drivers_out = waveOutGetNumDevs();
    int32_t drivers_total = waveInGetNumDevs() + drivers_out;

    *devicelistchanged = drivers_total != lastNumDrivers && lastNumDrivers - 1;
    lastNumDrivers = drivers_total;

    return FMOD_OK;
}
