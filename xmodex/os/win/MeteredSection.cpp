#include "xmodex/os/win/MeteredSection.hpp"
#include "xmodex/Memory.hpp"
#include "xmodex/String.hpp"
#include "xmodex/Math.hpp"
#include <windows.h>

int32_t CreateMetSectEvent(LPMETERED_SECTION lpMetSect, const char* lpName, int32_t bOpenOnly) {
    char sz[260];

    if (lpName) {
        if (bOpenOnly) {
            lpMetSect->hEvent = OpenEvent(0, 0, sz);
        } else {
            // Create an auto-reset named event object
            lpMetSect->hEvent = CreateEvent(nullptr, 0, 0, sz);
        }
    } else {
        // Create an auto-reset unnamed event object
        lpMetSect->hEvent = CreateEvent(nullptr, 0, 0, nullptr);
    }

    return lpMetSect->hEvent ? 1 : 0;
}

int32_t CreateMetSectFileView(
    LPMETERED_SECTION lpMetSect,
    int32_t lInitialCount,
    int32_t lMaximumCount,
    const char* lpName,
    int32_t bOpenOnly) {

    char sz[260];
    DWORD dwLastError;

    if (lpName) {
        if (bOpenOnly) {
            lpMetSect->hFileMap = OpenFileMapping(0, 0, sz);
        } else {
            // Create a named file mapping
            lpMetSect->hFileMap = CreateFileMapping(
                INVALID_HANDLE_VALUE,
                nullptr,
                PAGE_READWRITE,
                0,
                sizeof(METSECT_SHARED_INFO),
                sz);
        }
    } else {
        lpMetSect->hFileMap = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(METSECT_SHARED_INFO),
            nullptr);
    }

    if (lpMetSect->hFileMap) {
        dwLastError = GetLastError();
        lpMetSect->lpSharedInfo = static_cast<LPMETSECT_SHARED_INFO>(MapViewOfFile(
            lpMetSect->hFileMap,
            FILE_MAP_WRITE,
            0,
            0,
            0));

        if (lpMetSect->lpSharedInfo) {
            if (dwLastError != ERROR_ALREADY_EXISTS) {
                lpMetSect->lpSharedInfo->lSpinLock = 0;
                lpMetSect->lpSharedInfo->lThreadsWaiting = 0;
                lpMetSect->lpSharedInfo->lAvailableCount = lInitialCount;
                lpMetSect->lpSharedInfo->lMaximumCount = lMaximumCount;
                InterlockedExchange(reinterpret_cast<LONG*>(&lpMetSect->lpSharedInfo->fInitialized), 1);
            } else {
                // Already exists; wait for it to be initialized by the creator
                while (!lpMetSect->lpSharedInfo->fInitialized) {
                    Sleep(0);
                }
            }

            return 1;
        }
    }

    return 0;
}

int32_t InitMeteredSection(
    LPMETERED_SECTION lpMetSect,
    int32_t lInitialCount,
    int32_t lMaximumCount,
    const char* lpName,
    int32_t bOpenOnly) {

    // Try to create the event object
    if (CreateMetSectEvent(lpMetSect, lpName, bOpenOnly)) {
        // Try to create the memory mapped file
        if (CreateMetSectFileView(lpMetSect, lInitialCount, lMaximumCount, lpName, bOpenOnly)) {
            return 1;
        }
    }

    return 0;
}

void CloseMeteredSection(LPMETERED_SECTION lpMetSect) {
    if (lpMetSect) {
        // Clean up
        if (lpMetSect->lpSharedInfo) {
            UnmapViewOfFile(lpMetSect->lpSharedInfo);
        }

        if (lpMetSect->hFileMap) {
            CloseHandle(lpMetSect->hFileMap);
        }

        if (lpMetSect->hEvent) {
            CloseHandle(lpMetSect->hEvent);
        }

        FMOD_Memory_Free(lpMetSect);
    }
}

LPMETERED_SECTION CreateMeteredSection(
    int32_t lInitialCount,
    int32_t lMaximumCount,
    const char* lpName) {

    LPMETERED_SECTION lpMetSect;

    // Verify the parameters
    if ((lMaximumCount < 1) ||
        (lInitialCount > lMaximumCount) ||
        (lInitialCount < 0) ||
        ((lpName) && (FMOD_strlen(lpName) > MAX_METSECT_NAMELEN))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    // Allocate memory for the metered section
    lpMetSect = static_cast<LPMETERED_SECTION>(FMOD_Memory_Alloc(sizeof(METERED_SECTION)));

    // If the memory for the metered section was allocated okay,
    // initialize it
    if (lpMetSect) {
        if (!InitMeteredSection(
                lpMetSect,
                lInitialCount,
                lMaximumCount,
                lpName,
                0)) {
            CloseMeteredSection(lpMetSect);
            lpMetSect = nullptr;
        }
    }

    return lpMetSect;
}

void GetMeteredSectionLock(LPMETERED_SECTION lpMetSect) {
    // Spin and get access to the metered section lock
    while (InterlockedExchange(reinterpret_cast<LONG*>(&lpMetSect->lpSharedInfo->lSpinLock), 1) != 0) {
        Sleep(1);
    }
}

void ReleaseMeteredSectionLock(LPMETERED_SECTION lpMetSect) {
    InterlockedExchange(reinterpret_cast<LONG*>(&lpMetSect->lpSharedInfo->lSpinLock), 0);
}

uint32_t EnterMeteredSection(
    LPMETERED_SECTION lpMetSect,
    uint32_t dwMilliseconds) {
    while (1) {
        GetMeteredSectionLock(lpMetSect);

        // We have access to the metered section, everything we
        // do now will be atomic
        if (lpMetSect->lpSharedInfo->lAvailableCount >= 1) {
            lpMetSect->lpSharedInfo->lAvailableCount--;
            ReleaseMeteredSectionLock(lpMetSect);
            return WAIT_OBJECT_0;
        }

        // Couldn't get in. Wait on the event object
        lpMetSect->lpSharedInfo->lThreadsWaiting++;
        ResetEvent(lpMetSect->hEvent);
        ReleaseMeteredSectionLock(lpMetSect);
        if (WaitForSingleObject(
                lpMetSect->hEvent,
                dwMilliseconds) == WAIT_TIMEOUT) {
            return WAIT_TIMEOUT;
        }
    }
}

int32_t LeaveMeteredSection(
    LPMETERED_SECTION lpMetSect,
    int32_t lReleaseCount,
    int32_t* lpPreviousCount) {

    int32_t iCount;
    GetMeteredSectionLock(lpMetSect);

    // Save the old value if they want it
    if (lpPreviousCount) {
        *lpPreviousCount = lpMetSect->lpSharedInfo->lAvailableCount;
    }

    // We have access to the metered section,
    // everything we do now will be atomic
    if ((lReleaseCount < 0) ||
        (lpMetSect->lpSharedInfo->lAvailableCount + lReleaseCount >
         lpMetSect->lpSharedInfo->lMaximumCount)) {
        ReleaseMeteredSectionLock(lpMetSect);
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    lpMetSect->lpSharedInfo->lAvailableCount += lReleaseCount;

    // Set the event the appropriate number of times
    lReleaseCount = FMOD_MIN(lReleaseCount, lpMetSect->lpSharedInfo->lThreadsWaiting);
    if (lpMetSect->lpSharedInfo->lThreadsWaiting) {
        for (iCount = 0; iCount < lReleaseCount; iCount++) {
            lpMetSect->lpSharedInfo->lThreadsWaiting--;
            SetEvent(lpMetSect->hEvent);
        }
    }
    ReleaseMeteredSectionLock(lpMetSect);
    return 1;
}
