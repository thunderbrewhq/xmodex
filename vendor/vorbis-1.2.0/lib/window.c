/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: window functions
 last mod: $Id: window.c 13293 2007-07-24 00:09:47Z xiphmont $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "os.h"
#include "misc.h"

static float vwin64[32];

static float vwin128[64];

static float vwin256[128];

static float vwin512[256];

static float vwin1024[512];

static float vwin2048[1024];

static float vwin4096[2048];

static float vwin8192[4096];

static float* vwin[8] = {
    vwin64,
    vwin128,
    vwin256,
    vwin512,
    vwin1024,
    vwin2048,
    vwin4096,
    vwin8192,
};

void _vorbis_window_init() {
    int i;
    int left = 32;

    while (left <= 4096) {
        float* tab = 0;

        switch (left) {
        case 32:
            tab = vwin64;
            break;
        case 64:
            tab = vwin128;
            break;
        case 128:
            tab = vwin256;
            break;
        case 256:
            tab = vwin512;
            break;
        case 512:
            tab = vwin1024;
            break;
        case 1024:
            tab = vwin2048;
            break;
        case 2048:
            tab = vwin4096;
            break;
        case 4096:
            tab = vwin8192;
            break;
        };

        if (tab) {
            for (i = 0; i < left; i++) {
                float x = (i + .5f) / left * M_PI / 2.f;
                x = (float)sin(x);
                x *= x;
                x *= M_PI / 2.f;
                x = (float)sin(x);
                tab[i] = x;
            }
        }

        left <<= 1;
    }
}

float* _vorbis_window_get(int n) {
    return vwin[n];
}

void _vorbis_apply_window(float* d, int* winno, long* blocksizes,
                          int lW, int W, int nW) {
    lW = (W ? lW : 0);
    nW = (W ? nW : 0);

    {
        float* windowLW = vwin[winno[lW]];
        float* windowNW = vwin[winno[nW]];

        long n = blocksizes[W];
        long ln = blocksizes[lW];
        long rn = blocksizes[nW];

        long leftbegin = n / 4 - ln / 4;
        long leftend = leftbegin + ln / 2;

        long rightbegin = n / 2 + n / 4 - rn / 4;
        long rightend = rightbegin + rn / 2;

        int i, p;

        for (i = 0; i < leftbegin; i++)
            d[i] = 0.f;

        for (p = 0; i < leftend; i++, p++)
            d[i] *= windowLW[p];

        for (i = rightbegin, p = rn / 2 - 1; i < rightend; i++, p--)
            d[i] *= windowNW[p];

        for (; i < n; i++)
            d[i] = 0.f;
    }
}
