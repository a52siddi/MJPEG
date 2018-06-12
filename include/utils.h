/*
 * File   : utils.h, file for JPEG-JFIF Multi-thread decoder
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

static inline int32_t intceil (int32_t N, int32_t D)
{
int32_t i = N / D;

	if (N > D * i) i++;
	return i;
}

static inline int32_t intfloor (int32_t N, int32_t D)
{
	int32_t i = N / D;

	if (N < D * i) i--;
	return i;
}

/* compute an actual value from an class of magnitude "good" and the
 * value S (section "Arbres DC et codage DPCM" in mjpeg.tex) */
static inline int32_t reformat (uint32_t S, int32_t good)
{
	int32_t St = 0;

	if (good == 0) return 0;
	St = 1 << (good - 1);   /* 2^(good-1) */

	if (S < St) return (S + 1 + ((-1) << good));
	else return S;
}
#endif
