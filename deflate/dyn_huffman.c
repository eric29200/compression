#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_huffman.h"
#include "huffman.h"
#include "../huffman/huffman_tree.h"
#include "../utils/heap.h"
#include "../utils/mem.h"

/**
 * @brief Build dynamic huffman tables.
 * 
 * @param lz77_nodes		LZ77 nodes
 * @param table_lit 		literals huffman table
 * @param table_dist 		distances huffman table
 */
void deflate_huffman_build_dynamic_tables(struct lz77_node *lz77_nodes, struct huffman_table *table_lit, struct huffman_table *table_dist)
{
	uint32_t freqs_lit[NR_LITERALS] = { 0 }, freqs_dist[NR_DISTANCES] = { 0 };
	struct huffman_node *tree_lit, *tree_dist;
	struct lz77_node *lz77_node;

	/* compute literals and distances frequencies */
	for (lz77_node = lz77_nodes; lz77_node != NULL; lz77_node = lz77_node->next) {
		if (lz77_node->is_literal) {
			freqs_lit[lz77_node->data.literal]++;
		} else {
			freqs_lit[257 + deflate_huffman_length_index(lz77_node->data.match.length)]++;
			freqs_dist[deflate_huffman_distance_index(lz77_node->data.match.distance)]++;
		}
	}

	/* add "end of block" character */
	freqs_lit[256]++;

	/* build huffman trees */
	tree_lit = huffman_tree_create(freqs_lit, NR_LITERALS);
	tree_dist = huffman_tree_create(freqs_dist, NR_DISTANCES);

	/* build huffman tables */
	huffman_table_build(tree_lit, table_lit, NR_LITERALS);
	huffman_table_build(tree_dist, table_dist, NR_DISTANCES);

	/* free huffman trees */
	huffman_tree_free(tree_lit);
	huffman_tree_free(tree_dist);
}

/**
 * @brief Write huffman tables.
 * 
 * @param bs_out 	output bit stream
 * @param table_lit 	huffman literals table
 * @param table_dist	huffman distances table
 */
void deflate_huffman_write_tables(struct bit_stream *bs_out, struct huffman_table *table_lit, struct huffman_table *table_dist)
{
	uint32_t i;

	/* write literals table */
	for (i = 0; i < table_lit->len; i++) {
		bit_stream_write_bits(bs_out, table_lit->codes_len[i], 9, BIT_ORDER_MSB);
		if (table_lit->codes_len[i])
			bit_stream_write_bits(bs_out, table_lit->codes[i], table_lit->codes_len[i], BIT_ORDER_MSB);
	}

	/* write distances table */
	for (i = 0; i < table_dist->len; i++) {
		bit_stream_write_bits(bs_out, table_dist->codes_len[i], 9, BIT_ORDER_MSB);
		if (table_dist->codes_len[i])
			bit_stream_write_bits(bs_out, table_dist->codes[i], table_dist->codes_len[i], BIT_ORDER_MSB);
	}
}

/**
 * @brief Read huffman tables.
 * 
 * @param bs_in 	input bit stream
 * @param table_lit 	huffman literals table
 * @param table_dist	huffman distances table
 */
void deflate_huffman_read_tables(struct bit_stream *bs_in, struct huffman_table *table_lit, struct huffman_table *table_dist)
{
	uint32_t i;

	/* read literals table */
	huffman_table_create(table_lit, NR_LITERALS);
	for (i = 0; i < table_lit->len; i++) {
		table_lit->codes_len[i] = bit_stream_read_bits(bs_in, 9, BIT_ORDER_MSB);
		if (table_lit->codes_len[i])
			table_lit->codes[i] = bit_stream_read_bits(bs_in, table_lit->codes_len[i], BIT_ORDER_MSB);
		else
			table_lit->codes[i] = 0;
	}

	/* read distances table */
	huffman_table_create(table_dist, NR_DISTANCES);
	for (i = 0; i < table_dist->len; i++) {
		table_dist->codes_len[i] = bit_stream_read_bits(bs_in, 9, BIT_ORDER_MSB);
		if (table_dist->codes_len[i])
			table_dist->codes[i] = bit_stream_read_bits(bs_in, table_dist->codes_len[i], BIT_ORDER_MSB);
		else
			table_dist->codes[i] = 0;
	}
}