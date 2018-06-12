/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "skip_segment.h"
#include "define_common.h"

void skip_segment(FILE * movie)
{
	union {
		uint16_t segment_size;
		uint8_t size[2];
	} u;

	/* tokens are followed by the size of their section, on 16bits */
	NEXT_TOKEN(u.size[0]);
	NEXT_TOKEN(u.size[1]);
	CPU_DATA_IS_BIGENDIAN(16, u.segment_size);

	IPRINTF("Skip segment (%d data)\r\n", (unsigned int) u.segment_size);
	SKIP(u.segment_size - 2);
}

