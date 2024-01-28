#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_huffman.h"
#include "huffman.h"
#include "../huffman/huffman_tree.h"
#include "../utils/heap.h"
#include "../utils/mem.h"

#define NR_LENGTHS_LEN		19

/**
 * @brief Lengths orders.
 */
static int __len_order[] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

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
	huffman_table_build_from_tree(tree_lit, table_lit, NR_LITERALS);
	huffman_table_build_from_tree(tree_dist, table_dist, NR_DISTANCES);

	/* free huffman trees */
	huffman_tree_free(tree_lit);
	huffman_tree_free(tree_dist);
}

/**
 * @brief Pack literals and distances codes lengths.
 * 
 * @param codes_len		input codes lengths
 * @param nr_codes		number of input codes
 * @param codes_len_packed	output codes lengths packed
 *
 * @return number of codes len packed
 */
static uint32_t __pack_codes_len(uint32_t *codes_len, uint32_t nr_codes, uint32_t *codes_len_packed)
{
	uint32_t run_length, last, i, j, n;

	for (i = 1, n = 0, run_length = 1, last = codes_len[0]; i <= nr_codes; i++) {
		/* continue run length sequence */
		if (i < nr_codes && codes_len[i] == last) {
			run_length++;
			continue;
		}

		/* end run length sequence */
		codes_len_packed[n++] = last;
		run_length--;

		/*
		 * 0 sequence :
		 * - code 18 = repeat from 11 to 138
		 * - code 17 = repeat from 3 to 10
		 */
		if (last == 0) {
			/* repeat 0 length (from 11 to 138) */
			for (j = 138; j >= 11;) {
				if (run_length >= j) {
					codes_len_packed[n++] = 18;
					codes_len_packed[n++] = j - 11;
					run_length -= j;
				} else {
					j--;
				}
			}

			/* repeat 0 length (from 3 to 10) */
			while (j >= 3) {
				if (run_length >= j) {
					codes_len_packed[n++] = 17;
					codes_len_packed[n++] = j - 3;
					run_length -= j;
				} else {
					j--;
				}
			}

			goto next;
		}

		/* repeat previous length from 3 to 6 */
		for (j = 6; j >= 3;) {
			if (run_length >= j) {
				codes_len_packed[n++] = 16;
				codes_len_packed[n++] = j - 3;
				run_length -= j;
			} else {
				j--;
			}
		}

next:
		/* add remaining lengths */
		while (run_length > 0) {
			codes_len_packed[n++] = last;
			run_length--;
		}

		/* update last length */
		if (i < nr_codes) {
			last = codes_len[i];
			run_length = 1;
		}
	}

	return n;
}

/**
 * @brief Unpack literals and distances codes lengths.
 * 
 * @param bs_in 		input bit stream
 * @param nr_literals		number of literals codes
 * @param nr_distances		number of distances codes
 * @param table_len_codes 	huffman table
 * @param codes_len 		output literals and distances codes lengths
 */
static void __unpack_codes_len(struct bit_stream *bs_in, uint32_t nr_literals, uint32_t nr_distances, struct huffman_table *table_len_codes, uint32_t *codes_len)
{
	uint32_t symbol, i, j, n;

	/* decode literals/distances tables */
	for (i = 0; i < nr_literals + nr_distances; i++) {
		/* read next symbol */
		symbol = huffman_table_read_symbol(bs_in, table_len_codes);

		switch (symbol) {
			case 16:							/* repeat previous length (from 3 to 6 ) */
				n = 3 + bit_stream_read_bits(bs_in, 2, BIT_ORDER_LSB);
				for (j = 0; j < n; j++)
					codes_len[i + j] = codes_len[i - 1];
				i += n - 1;
				break;
			case 17:							/* repeat 0 length (from 3 to 10) */
				n = 3 + bit_stream_read_bits(bs_in, 3, BIT_ORDER_LSB);
				for (j = 0; j < n; j++)
					codes_len[i + j] = 0;
				i += n - 1;
				break;
			case 18:							/* repeat 0 length (from 11 to 138) */
				n = 11 + bit_stream_read_bits(bs_in, 7, BIT_ORDER_LSB);
				for (j = 0; j < n; j++)
					codes_len[i + j] = 0;
				i += n - 1;
				break;
			default:
				codes_len[i] = symbol;
				break;
		}
	}
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
	uint32_t lengths[NR_LITERALS + NR_DISTANCES] = { 0 }, freqs_len[NR_LENGTHS_LEN] = { 0 }, lengths_len, i;
	struct huffman_table table_len;
	struct huffman_node *tree_len;

	/* write number of literals, distances and lengths */
	bit_stream_write_bits(bs_out, NR_LITERALS - 257, 5, BIT_ORDER_LSB);
	bit_stream_write_bits(bs_out, NR_DISTANCES - 1, 5, BIT_ORDER_LSB);
	bit_stream_write_bits(bs_out, NR_LENGTHS_LEN - 4, 4, BIT_ORDER_LSB);

	/* pack codes lengths */
	lengths_len = __pack_codes_len(table_lit->codes_len, table_lit->len, lengths);
	lengths_len += __pack_codes_len(table_dist->codes_len, table_dist->len, &lengths[lengths_len]);

	/* compute lengths frequencies */
	for (i = 0; i < lengths_len; i++) {
		freqs_len[lengths[i]]++;

		/* skip rle */
		if (lengths[i] == 16 || lengths[i] == 17 || lengths[i] == 18)
			i++;
	}

	/* build huffman table */
	tree_len = huffman_tree_create(freqs_len, NR_LENGTHS_LEN);
	huffman_table_build_from_tree(tree_len, &table_len, NR_LENGTHS_LEN);
	
	/* write length codes lengths */
	for (i = 0; i < NR_LENGTHS_LEN; i++)
		bit_stream_write_bits(bs_out, table_len.codes_len[__len_order[i]], 3, BIT_ORDER_LSB);
	
	/* write length codes */
	for (i = 0; i < lengths_len; i++) {
		bit_stream_write_bits(bs_out, table_len.codes[lengths[i]], table_len.codes_len[lengths[i]], BIT_ORDER_MSB);
		if (lengths[i] == 16)
			bit_stream_write_bits(bs_out, lengths[++i], 2, BIT_ORDER_LSB);
		else if (lengths[i] == 17)
			bit_stream_write_bits(bs_out, lengths[++i], 3, BIT_ORDER_LSB);
		else if (lengths[i] == 18)
			bit_stream_write_bits(bs_out, lengths[++i], 7, BIT_ORDER_LSB);
	}

	/* free huffman table */
	huffman_table_free(&table_len);
	huffman_tree_free(tree_len);
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
	uint32_t len_codes_len[NR_LENGTHS] = { 0 }, codes_len[NR_LITERALS + NR_DISTANCES] = { 0 };
	uint32_t nr_literals, nr_distances, nr_lengths, i;
	struct huffman_table table_len_codes;

	/* read number of literals, distances and lengths */
	nr_literals = 257 + bit_stream_read_bits(bs_in, 5, BIT_ORDER_LSB);
	nr_distances = 1 + bit_stream_read_bits(bs_in, 5, BIT_ORDER_LSB);
	nr_lengths = 4 + bit_stream_read_bits(bs_in, 4, BIT_ORDER_LSB);

	/* read length codes lengths */
	for (i = 0; i < nr_lengths; i++)
		len_codes_len[__len_order[i]] = bit_stream_read_bits(bs_in, 3, BIT_ORDER_LSB);

	/* build temporary huffman table */
	huffman_table_build_from_lengths(len_codes_len, nr_lengths, &table_len_codes);

	/* unpack literals/distances codes lengths */
	__unpack_codes_len(bs_in, nr_literals, nr_distances, &table_len_codes, codes_len);

	/* build huffman tables */
	huffman_table_build_from_lengths(codes_len, nr_literals, table_lit);
	huffman_table_build_from_lengths(&codes_len[nr_literals], nr_distances, table_dist);

	/* free temporary table */
	huffman_table_free(&table_len_codes);
}
