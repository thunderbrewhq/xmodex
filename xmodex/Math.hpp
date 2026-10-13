#ifndef XMODEX_MATH_HPP
#define XMODEX_MATH_HPP

#include "xmodex/Types.hpp"
#include <cmath>
#include <cstdint>

#define FMOD_PI (3.14159265358979323846f)
#define FMOD_PI2 (3.14159265358979323846f * 2.0f)
#define FMOD_PI_2 (3.14159265358979323846f / 2.0f)
#define FMOD_SQRT2 (1.41421356237309504880f)

#define FMOD_ABS(_x) ((_x) < 0 ? -(_x) : (_x))
#define FMOD_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define FMOD_MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))

#define FMOD_RAND rand

#define FMOD_SIN (float)sin
#define FMOD_SINH (float)sinh
#define FMOD_COS (float)cos
#define FMOD_TAN (float)tan
#define FMOD_SQRT (float)sqrt
#define FMOD_POW (float)pow
#define FMOD_ACOS (float)acos
#define FMOD_ATAN (float)atan
#define FMOD_EXP (float)exp
#define FMOD_LDEXP (float)ldexp
#define FMOD_FABS fabsf
#define FMOD_LOG (float)log
#define FMOD_LOG10 (float)log10
#define FMOD_FMOD (float)fmod
#define FMOD_FLOOR (float)floor

// Vector functions

#define FMOD_Vector_CrossProduct(_a, _b, _r)             \
    (_r)->x = ((_a)->y * (_b)->z) - ((_a)->z * (_b)->y); \
    (_r)->y = ((_a)->z * (_b)->x) - ((_a)->x * (_b)->z); \
    (_r)->z = ((_a)->x * (_b)->y) - ((_a)->y * (_b)->x);

#define FMOD_Vector_Subtract(_a, _b, _r) \
    (_r)->x = (_a)->x - (_b)->x;         \
    (_r)->y = (_a)->y - (_b)->y;         \
    (_r)->z = (_a)->z - (_b)->z;

#define FMOD_Vector_Add(_a, _b, _r) \
    (_r)->x = (_a)->x + (_b)->x;    \
    (_r)->y = (_a)->y + (_b)->y;    \
    (_r)->z = (_a)->z + (_b)->z;

#define FMOD_Vector_Scale(_a, _s, _r) \
    (_r)->x = ((_a)->x * (_s));       \
    (_r)->y = ((_a)->y * (_s));       \
    (_r)->z = ((_a)->z * (_s));

#define FMOD_Vector_Copy(_a, _r) \
    (_r)->x = (_a)->x;           \
    (_r)->y = (_a)->y;           \
    (_r)->z = (_a)->z;

#define FMOD_Vector_DotProduct(_a, _b) \
    (((_a)->x * (_b)->x) + ((_a)->y * (_b)->y) + ((_a)->z * (_b)->z))

#define FMOD_Vector_GetLength(_x) FMOD_SQRT(FMOD_Vector_DotProduct((_x), (_x)))

// Angle

static inline int32_t FMOD_Cart2Angle(int32_t y, int32_t x) {
    constexpr int32_t SHIFT = 10;
    constexpr int32_t M_PII = 3216;
    int32_t coeff_1;
    int32_t coeff_2;
    int32_t abs_y;
    int32_t r;
    int32_t angle;

    if (!x && !y) {
        return 0;
    }

    x <<= SHIFT;
    y <<= SHIFT;
    coeff_1 = M_PII / 4;
    coeff_2 = 3 * coeff_1;
    abs_y = abs(y) + 1;
    if (x >= 0) {
        r = (x - abs_y) / ((x + abs_y) >> SHIFT);
        angle = coeff_1 - ((coeff_1 * r) >> SHIFT);
    } else {
        r = (x + abs_y) / ((abs_y - x) >> SHIFT);
        angle = coeff_2 - ((coeff_1 * r) >> SHIFT);
    }
    if (y < 0) {
        angle = -angle;
    }
    angle *= 180;
    angle /= M_PII;
    angle = (angle < 0) ? angle + 360 : (angle >= 360) ? angle - 360
                                                       : angle;
    return angle;
}

// Hacks

#define FMOD_ALIGNPOINTER(_ptr, _align) ((((FMOD_UINT_NATIVE)(_ptr)) + ((_align) - 1)) & ~((_align) - 1))

#define FMOD_SWAPENDIAN_WORD(_x) (((static_cast<uint16_t>(_x) & 0xFF00) >> 8) | \
                                  ((static_cast<uint16_t>(_x) & 0x00FF) << 8))

#define FMOD_SWAPENDIAN_DWORD(_x) (((static_cast<uint32_t>(_x) & 0x000000FF) << 24) | \
                                   ((static_cast<uint32_t>(_x) & 0x0000FF00) << 8) |  \
                                   ((static_cast<uint32_t>(_x) & 0x00FF0000) >> 8) |  \
                                   ((static_cast<uint32_t>(_x) & 0xFF000000) >> 24))

#define FMOD_SWAPENDIAN_QWORD(_x) (((static_cast<FMOD_UINT64>(_x) & 0x00000000000000FFULL) << 56) | \
                                   ((static_cast<FMOD_UINT64>(_x) & 0x000000000000FF00ULL) << 40) | \
                                   ((static_cast<FMOD_UINT64>(_x) & 0x0000000000FF0000ULL) << 24) | \
                                   ((static_cast<FMOD_UINT64>(_x) & 0x00000000FF000000ULL) << 8) |  \
                                   ((static_cast<FMOD_UINT64>(_x) & 0x000000FF00000000ULL) >> 8) |  \
                                   ((static_cast<FMOD_UINT64>(_x) & 0x0000FF0000000000ULL) >> 24) | \
                                   ((static_cast<FMOD_UINT64>(_x) & 0x00FF000000000000ULL) >> 40) | \
                                   ((static_cast<FMOD_UINT64>(_x) & 0xFF00000000000000ULL) >> 56))

// Cryptography

#define FMOD_DECRYPT(x, y)          \
    {                               \
        uint8_t __tmp = (x) ^ (y);  \
        (x) = 0;                    \
        (x) |= (__tmp & 0x01) << 7; \
        (x) |= (__tmp & 0x02) << 5; \
        (x) |= (__tmp & 0x04) << 3; \
        (x) |= (__tmp & 0x08) << 1; \
        (x) |= (__tmp & 0x10) >> 1; \
        (x) |= (__tmp & 0x20) >> 3; \
        (x) |= (__tmp & 0x40) >> 5; \
        (x) |= (__tmp & 0x80) >> 7; \
    }

// Randomness

inline float FMOD_Random_Value(float r) {
    return (static_cast<float>(rand() % 32768) / (16384.0f / r)) - r;
}

#endif
