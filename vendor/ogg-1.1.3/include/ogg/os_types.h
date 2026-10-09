/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.
 last mod: $Id: os_types.h 7524 2004-08-11 04:20:36Z conrad $

 ********************************************************************/
#ifndef _OS_TYPES_H
#define _OS_TYPES_H

#include <stdint.h>

typedef int16_t ogg_int16_t;
typedef uint16_t ogg_uint16_t;
typedef int32_t ogg_int32_t;
typedef uint32_t ogg_uint32_t;
typedef int64_t ogg_int64_t;
typedef uint64_t ogg_uint64_t;

void* FMOD_Memory_allocC(int32_t len, const char* file, const int32_t line);
void* FMOD_Memory_callocC(int32_t len, const char* file, const int32_t line);
void* FMOD_Memory_reallocC(void* ptr, int32_t len, const char* file, const int32_t line);
void FMOD_Memory_freeC(void* ptr, const char* file, const int32_t line);

#define _ogg_malloc(_x) FMOD_Memory_allocC((_x), __FILE__, __LINE__)
#define _ogg_calloc(_x, _y) FMOD_Memory_callocC((_x) * (_y), __FILE__, __LINE__)
#define _ogg_realloc(_x, _y) FMOD_Memory_reallocC((_x), _y, __FILE__, __LINE__)
#define _ogg_free(_x) FMOD_Memory_freeC((_x), __FILE__, __LINE__)

#endif /* _OS_TYPES_H */
