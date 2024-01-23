#include <stdio.h>
#include <string.h>

#include "fix_huffman.h"
#include "huffman.h"
#include "../utils/mem.h"

/**
 * @brief Build fix huffman tables.
 * 
 * @param table_lit 		literals huffman table
 * @param table_dist 		distances huffman table
 */
void deflate_huffman_build_fix_tables(struct huffman_table *table_lit, struct huffman_table *table_dist)
{
	int i, j;

	/* allocate literals table */
	memset(table_lit, 0, sizeof(struct huffman_table));
	table_lit->len = NR_LITERALS;

	/* allocate distances table */
	memset(table_dist, 0, sizeof(struct huffman_table));
	table_dist->len = NR_DISTANCES;

	/*
	 * - values from 256 to 279 = codes from 0 to 23 on 7 bits
	 */
	for (i = 256, j = 0; i <= 279; i++, j++) {
		table_lit->codes[i] = j;
		table_lit->codes_len[i] = 7;
	}

	/*
	 * - values from 0 to 143 = codes from 48 to 191 on 8 bits
	 */
	for (i = 0, j = 48; i <= 143; i++, j++) {
		table_lit->codes[i] = j;
		table_lit->codes_len[i] = 8;
	}

	/*
	 * - values from 280 to 287 = codes from 192 to 199 on 8 bits
	 */
	for (i = 280, j = 192; i <= 285; i++, j++) {
		table_lit->codes[i] = j;
		table_lit->codes_len[i] = 8;
	}

	/*
	 * - values from 144 to 255 = codes from 400 to 511 on 9 bits
	 */
	for (i = 144, j = 400; i <= 255; i++, j++) {
		table_lit->codes[i] = j;
		table_lit->codes_len[i] = 9;
	}

	/*
	 * - distances from 0 to 29 = codes from 0 to 29 on 5 bits
	 */
	for (i = 0; i < NR_DISTANCES; i++) {
		table_dist->codes[i] = i;
		table_dist->codes_len[i] = 5;
	}
}