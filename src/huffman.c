/*************************************************************************************
 * vim :set ts=8:
 ******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "huffman.h"
#include "define_common.h"


/*
 * Tree version of huffman tables management
 * Enjoy !
 * expansion is unnecessary, but is still present just in case
 */

void free_huffman_tables(huff_table_t *root) {
	if (root == NULL) {
//		fprintf(stderr, "%s : can't free NULL tree ! \n", __func__);
		return;
	}
	if (root->left != NULL) {
		free_huffman_tables(root->left);
		root->left = NULL;
	}
	if (root->right != NULL) {
		free_huffman_tables(root->right);
		root->right = NULL;
	}
	free(root);
	root = NULL;
}

// we need to find the first location at depth depth in the table
huff_table_t *find_location(huff_table_t *root, uint16_t depth) {
	huff_table_t *return_ptr = NULL;
	if (root->is_elt == 0) {
		if (depth == 0) {
			return root;
		}
		if (root->left == NULL) {
			root->left = (huff_table_t *)malloc(sizeof(huff_table_t));
			root->left->value = 0;
			root->left->code = (root->code) << 1;
			root->left->is_elt = 0;
			root->left->left = NULL;
			root->left->right = NULL;
			root->left->parent = root;
		}
		return_ptr = find_location(root->left, depth-1);
		if (return_ptr != NULL) {
			return return_ptr;
		}
		if (root->right == NULL) {
			root->right = (huff_table_t *)malloc(sizeof(huff_table_t));
			root->right->value = 0;
			root->right->code = ((root->code) << 1)| 1;
			root->right->is_elt = 0;
			root->right->left = NULL;
			root->right->right = NULL;
			root->right->parent = root;
		}
		return_ptr = find_location(root->right, depth-1);
		if (return_ptr != NULL) {
			return return_ptr;
		}
	} else {
		return NULL;
	}
	return NULL;
}

int load_huffman_table(FILE * movie, huff_table_t * ht)
{
	uint8_t buffer = 0;
	int32_t size = 0, i = 0, j = 0;
	uint8_t num_values[16];
	huff_table_t *current_ptr = NULL;
	// ht est la racine de l'arbre
	// rien d'autre n'est alloué !!!

	ht->code = 0;
	ht->value = 0;
	ht->is_elt = 0;
	ht->parent = NULL;
	ht->left = NULL;
	ht->right = NULL;
	// récupération des longueurs :
	for (i=0 ; i < 16 ; i++) {
		NEXT_TOKEN(buffer);
		num_values[i] = buffer;
	}
	size = 16;
	for (i = 0 ; i < 16 ; i++) {
		for (j = 0 ; j < num_values[i] ; j++) {
			current_ptr = find_location(ht, i+1);
			if(current_ptr == NULL) {
				printf("Oops ! No location found, exiting...\n");
				return -1;
			}
			size++;
			NEXT_TOKEN(buffer);
			current_ptr->value = buffer;
			APRINTF("Value %02x has code %02x(%d)\n", buffer, current_ptr->code,i+1);
			current_ptr->is_elt = 1;
		}
	}
	return (size);
}

/*
 * Ancienne version de load_huffman.
 * On ne devrait plus en avoir besoin, mais le main appelle encore
 * cette version.
 */
int load_huffman_table_size(FILE * movie,
	uint16_t DHT_section_length,
	uint8_t DHT_section_info,
	huff_table_t * ht)
{
    uint8_t buffer = 0;
    int32_t size = 0, i = 0, j = 0;
    uint8_t num_values[16];
    huff_table_t *current_ptr = NULL;
    // ht est la racine de l'arbre
    // rien d'autre n'est alloué !!!

    ht->code = 0;
    ht->value = 0;
    ht->is_elt = 0;
    ht->parent = NULL;
    ht->left = NULL;
    ht->right = NULL;
    // récupération des longueurs :
    for (i=0 ; i < 16 ; i++) {
	NEXT_TOKEN(buffer);
	num_values[i] = buffer;
    }
    size = 16;
    for (i = 0 ; i < 16 ; i++) {
	for (j = 0 ; j < num_values[i] ; j++) {
	    current_ptr = find_location(ht, i+1);
	    if(current_ptr == NULL) {
		printf("Oops ! No location found, exiting...\n");
		return -1;
	    }
	    size++;
	    NEXT_TOKEN(buffer);
	    current_ptr->value = buffer;
	    APRINTF("Value %02x has code %02x(%d)\n", buffer, current_ptr->code,i+1);
	    current_ptr->is_elt = 1;
	}
    }
    return (size);
}

