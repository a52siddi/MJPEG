/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "iqzz.h"

/*---- Localy used variables ----*/
const uint8_t G_ZZ[64] = {
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

void iqzz_block(int32_t in[64], int32_t out[64],
		uint8_t table[64])
{
	uint32_t index;

	for (index = 0; index < 64; index++)
	{
		out[G_ZZ[index]] = in[index] * table[index];
	}
}
