/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#ifndef __UNPACK_BLOCK_H
#define __UNPACK_BLOCK_H

#include "jpeg.h"
#include <stdint.h>

extern void unpack_block(FILE *movie, scan_desc_t *scan_desc,
		uint32_t index, int32_t T[64]);


#endif

