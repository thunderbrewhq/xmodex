#ifndef XMODEX_STRING_HPP
#define XMODEX_STRING_HPP

#include <cstdint>
#include <cstring>

#define FMOD_STRING_MAXPATHLEN 256
#define FMOD_STRING_MAXNAMELEN 256

#define FMOD_isspace(j) (j == ' ' || j == '\t' || j == '\n')
#define FMOD_isdigit(j) (j >= '0' && j <= '9')
#define FMOD_ishexdigit(j) (((j) >= '0' && (j) <= '9') || ((j) >= 'A' && (j) <= 'F') || ((j) >= 'a' && (j) <= 'f'))
#define FMOD_isalpha(j) ((((j) >= 'A') && ((j) <= 'Z')) || (((j) >= 'a') && ((j) <= 'z')))

#define FMOD_memset memset
#define FMOD_memcpy memmove

int32_t FMOD_memcmp(const void* cs, const void* ct, int32_t count);

void* FMOD_memmove(void* dest, const void* src, int32_t count);

int32_t FMOD_strlen(const char* s);

char* FMOD_strcpy(char* dest, const char* src);

char* FMOD_strncpy(char* dest, const char* src, int32_t count);

char* FMOD_strcat(char* dest, const char* src);

char* FMOD_strncat(char* dest, const char* src, int32_t count);

char FMOD_tolower(char in);

char* FMOD_strupr(char* string);

int32_t FMOD_strcmp(const char* string1, const char* string2);

int32_t FMOD_strncmp(const char* string1, const char* string2, int32_t len);

int32_t FMOD_stricmp(const char* string1, const char* string2);

int32_t FMOD_strnicmp(const char* string1, const char* string2, int32_t len);

char* FMOD_strdup(const char* src);

char* FMOD_strstr(const char* s1, const char* s2);

int32_t FMOD_strlenW(const uint16_t* s);

uint16_t* FMOD_strncpyW(uint16_t* dest, const uint16_t* src, int32_t count);

#endif
