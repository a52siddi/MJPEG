/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#ifndef __HUFFMAN_H
#define __HUFFMAN_H

#include "jpeg.h"
#include <stdint.h>
#include <stdio.h>

extern void free_huffman_tables(huff_table_t *root);

extern int load_huffman_table(FILE *movie, huff_table_t *ht);

/*
 * Ancienne version de load_huffman.
 * On ne devrait plus en avoir besoin, mais le main appelle encore
 * cette version.
 */
extern int load_huffman_table_size(FILE *movie,
		uint16_t DHT_section_length,
		uint8_t DHT_section_info,
		huff_table_t *ht);

#endif
