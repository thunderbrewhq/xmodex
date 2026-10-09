#include "xmodex/String.hpp"
#include "xmodex/Memory.hpp"

int32_t FMOD_memcmp(const void* cs, const void* ct, int32_t count) {
    const uint8_t* su1;
    const uint8_t* su2;
    int8_t res = 0;

    for (su1 = static_cast<const uint8_t*>(cs), su2 = static_cast<const uint8_t*>(ct); 0 < count; ++su1, ++su2, count--) {
        if ((res = *su1 - *su2)) {
            break;
        }
    }

    return res;
}

void* FMOD_memmove(void* dest, const void* src, int32_t count) {
    char* tmp;
    const char* s;

    if (dest <= src) {
        tmp = static_cast<char*>(dest);
        s = static_cast<const char*>(src);
        while (count--) {
            *tmp++ = *s++;
        }
    } else {
        tmp = static_cast<char*>(dest) + count;
        s = static_cast<const char*>(src) + count;
        while (count--) {
            *--tmp = *--s;
        }
    }

    return dest;
}

int32_t FMOD_strlen(const char* s) {
    const char* sc;
    for (sc = s; *sc != '\0'; ++sc)
        ;

    return static_cast<int32_t>(sc - s);
}

char* FMOD_strcpy(char* dest, const char* src) {
    char* tmp = dest;

    while ((*dest++ = *src++) != '\0')
        ;

    return tmp;
}

char* FMOD_strncpy(char* dest, const char* src, int32_t count) {
    char* tmp = dest;

    while (count-- && (*dest++ = *src++) != '\0')
        ;

    return tmp;
}

char* FMOD_strcat(char* dest, const char* src) {
    char* tmp = dest;

    while (*dest)
        dest++;
    while ((*dest++ = *src++) != '\0')
        ;

    return tmp;
}

char* FMOD_strncat(char* dest, const char* src, int32_t count) {
    char* tmp = dest;

    if (count) {
        while (*dest)
            dest++;

        while ((*dest++ = *src++)) {
            if (--count == 0) {
                *dest = '\0';
                break;
            }
        }
    }

    return tmp;
}

char FMOD_tolower(char in) {
    if (in >= 'A' && in <= 'Z') {
        in += 'a' - 'A';
    }

    return in;
}

char* FMOD_strupr(char* string) {
    char* cp;

    for (cp = string; *cp; ++cp) {
        if (*cp >= 'a' && *cp <= 'z') {
            *cp += 'A' - 'a';
        }
    }

    return (string);
}

int32_t FMOD_strcmp(const char* string1, const char* string2) {
    char c1;
    char c2;

    do {
        c1 = *string1++;
        c2 = *string2++;
    } while (c1 && (c1 == c2));

    return c1 - c2;
}

int32_t FMOD_strncmp(const char* string1, const char* string2, int32_t len) {
    char c1;
    char c2;
    int32_t count = 0;

    do {
        c1 = *string1++;
        c2 = *string2++;
        count++;
    } while (c1 && (c1 == c2) && count < len);

    return c1 - c2;
}

int32_t FMOD_stricmp(const char* string1, const char* string2) {
    char c1, c2;
    do {
        c1 = FMOD_tolower(*string1++);
        c2 = FMOD_tolower(*string2++);
    } while (c1 && (c1 == c2));

    return c1 - c2;
}

int32_t FMOD_strnicmp(const char* string1, const char* string2, int32_t len) {
    char c1;
    char c2;
    int32_t count = 0;

    do {
        c1 = FMOD_tolower(*string1++);
        c2 = FMOD_tolower(*string2++);
        count++;
    } while (c1 && (c1 == c2) && count < len);

    return c1 - c2;
}

char* FMOD_strdup(const char* src) {
    auto ret = static_cast<char*>(FMOD_Memory_Alloc(FMOD_strlen(src) + 1));
    if (ret) {
        FMOD_strcpy(ret, src);
    }

    return ret;
}

char* FMOD_strstr(const char* s1, const char* s2) {
    int l1, l2;

    l2 = FMOD_strlen(s2);

    if (!l2) {
        return (char*)s1;
    }

    l1 = FMOD_strlen(s1);

    while (l1 >= l2) {
        l1--;

        if (!FMOD_memcmp(s1, s2, l2))
            return (char*)s1;

        s1++;
    }
    return (char*)0;
}

int32_t FMOD_strlenW(const uint16_t* s) {
    const uint16_t* sc;

    for (sc = s; *sc != '\0'; ++sc)
        ;

    return static_cast<int32_t>(sc - s);
}

uint16_t* FMOD_strncpyW(uint16_t* dest, const uint16_t* src, int32_t count) {
    auto tmp = dest;

    while (count-- && (*dest++ = *src++) != '\0')
        ;

    return tmp;
}
