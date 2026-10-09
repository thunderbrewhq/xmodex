#ifndef XMODEX_TYPES_HPP
#define XMODEX_TYPES_HPP

#include <cmath>
#include <cstdint>
#include <cstring>

#if defined(XMODEX_SYSTEM_WIN)

#include <malloc.h>

#define FMOD_alloca _alloca

#else

#include <alloca.h>

#define FMOD_alloca alloca

#endif

typedef uintptr_t FMOD_UINT_NATIVE;
typedef intptr_t FMOD_SINT_NATIVE;

typedef uint64_t FMOD_UINT64;
typedef int64_t FMOD_SINT64;

union FMOD_UINT64P {
        FMOD_UINT64 mValue;
        struct {
                uint32_t mLo;
                uint32_t mHi;
        };
};

union FMOD_SINT64P {
        FMOD_SINT64 mValue;
        struct {
                uint32_t mLo;
                int32_t mHi;
        };
};

#pragma pack(push, 1)

struct FMOD_INT24 {
        uint8_t val[3];
};

#pragma pack(pop)

#endif
