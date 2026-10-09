#ifndef XMODEX_WIN_METERED_SECTION_HPP
#define XMODEX_WIN_METERED_SECTION_HPP

#define MAX_METSECT_NAMELEN 128

#include <cstdint>
// #include <windows.h>
typedef void* HANDLE;

struct METSECT_SHARED_INFO {
        int32_t fInitialized;
        int32_t lSpinLock;
        int32_t lThreadsWaiting;
        int32_t lAvailableCount;
        int32_t lMaximumCount;
};
typedef METSECT_SHARED_INFO* LPMETSECT_SHARED_INFO;

struct METERED_SECTION {
        HANDLE hEvent;
        HANDLE hFileMap;
        LPMETSECT_SHARED_INFO lpSharedInfo;
};
typedef METERED_SECTION* LPMETERED_SECTION;

LPMETERED_SECTION CreateMeteredSection(
    int32_t lInitialCount,
    int32_t lMaximumCount,
    const char* lpName);

uint32_t EnterMeteredSection(
    LPMETERED_SECTION lpMetSect,
    uint32_t dwMilliseconds);

int32_t LeaveMeteredSection(
    LPMETERED_SECTION lpMetSect,
    int32_t lReleaseCount,
    int32_t* lpPreviousCount);

void CloseMeteredSection(LPMETERED_SECTION lpMetSect);

#endif
