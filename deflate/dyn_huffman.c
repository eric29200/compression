#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_huffman.h"
#include "huffman.h"
#include "../huffman/huffman_tree.h"
#include "../utils/heap.h"
#include "../utils/mem.h"

#define NR_LITERALS	286
#define NR_DISTANCES	30

/**
 * @brief Huffman table.
 */
struct huffman_table {
	int			len;				/* table length */
	int 			codes[NR_LITERALS];		/* values to huffman codes */
	int 			codes_len[NR_LITERALS];		/* values to huffman codes lengths (= number of bits) */
};

/**
 * @brief Build dynamic huffman tables.
 * 
 * @param lz77_nodes		LZ77 nodes
 * @param table_lit 		literals huffman table
 * @param table_dist 		distances huffman table
 */
static void __build_tables(struct lz77_node *lz77_nodes, struct huffman_table *table_lit, struct huffman_table *table_dist)
{
	struct huff_node *nodes_dist[NR_DISTANCES] = { NULL }, *nodes_lit[NR_LITERALS] = { NULL };
	uint32_t freqs_lit[NR_LITERALS] = { 0 }, freqs_dist[NR_DISTANCES] = { 0 };
	struct huff_node *tree_lit, *tree_dist;
	struct lz77_node *lz77_node;
	uint32_t val;
	int i;

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

	/* extract huffman nodes */
	huffman_tree_extract_nodes(tree_lit, nodes_lit);
	huffman_tree_extract_nodes(tree_dist, nodes_dist);

	/* build literals table */
	memset(table_lit, 0, sizeof(struct huffman_table));
	table_lit->len = NR_LITERALS;
	for (i = 0; i < NR_LITERALS; i++) {
		if (nodes_lit[i]) {
			val = nodes_lit[i]->val;
			table_lit->codes[val] = nodes_lit[i]->huff_code;
			table_lit->codes_len[val] = nodes_lit[i]->nr_bits;
		}
	}

	/* build distances table */
	memset(table_dist, 0, sizeof(struct huffman_table));
	table_dist->len = NR_DISTANCES;
	for (i = 0; i < NR_DISTANCES; i++) {
		if (nodes_dist[i]) {
			val = nodes_dist[i]->val;
			table_dist->codes[val] = nodes_dist[i]->huff_code;
			table_dist->codes_len[val] = nodes_dist[i]->nr_bits;
		}
	}

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
static void __write_huffman_tables(struct bit_stream *bs_out, struct huffman_table *table_lit, struct huffman_table *table_dist)
{
	int i;

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
static void __read_huffman_tables(struct bit_stream *bs_in, struct huffman_table *table_lit, struct huffman_table *table_dist)
{
	int i;

	/* read literals table */
	table_lit->len = NR_LITERALS;
	for (i = 0; i < table_lit->len; i++) {
		table_lit->codes_len[i] = bit_stream_read_bits(bs_in, 9, BIT_ORDER_MSB);
		if (table_lit->codes_len[i])
			table_lit->codes[i] = bit_stream_read_bits(bs_in, table_lit->codes_len[i], BIT_ORDER_MSB);
		else
			table_lit->codes[i] = 0;
	}

	/* read distances table */
	table_dist->len = NR_DISTANCES;
	for (i = 0; i < table_dist->len; i++) {
		table_dist->codes_len[i] = bit_stream_read_bits(bs_in, 9, BIT_ORDER_MSB);
		if (table_dist->codes_len[i])
			table_dist->codes[i] = bit_stream_read_bits(bs_in, table_dist->codes_len[i], BIT_ORDER_MSB);
		else
			table_dist->codes[i] = 0;
	}
}

/**
 * @brief Write a literal.
 * 
 * @param literal 		literal
 * @param huff_table		huffman table
 * @param bs_out 		output bit stream
 */
static void __write_literal(uint8_t literal, struct huffman_table *huff_table, struct bit_stream *bs_out)
{
	bit_stream_write_bits(bs_out, huff_table->codes[literal], huff_table->codes_len[literal], BIT_ORDER_MSB);
}

/**
 * @brief Write a distance.
 * 
 * @param distance 		distance
 * @param huff_table		huffman table
 * @param bs_out 		output bit stream
 */
static void __write_distance(int distance, struct huffman_table *huff_table, struct bit_stream *bs_out)
{
	int i;

	/* get distance index */
	i = deflate_huffman_distance_index(distance);
	
	/* write distance index */
	bit_stream_write_bits(bs_out, huff_table->codes[i], huff_table->codes_len[i], BIT_ORDER_MSB);

	/* write distance extra bits */
	deflate_huffman_encode_distance_extra_bits(bs_out, distance);
}

/**
 * @brief Write a length.
 * 
 * @param length 		length
 * @param huff_table		huffman table
 * @param bs_out 		output bit stream
 */
static void __write_length(int length, struct huffman_table *huff_table, struct bit_stream *bs_out)
{
	int i;

	/* get length index */
	i = deflate_huffman_length_index(length) + 1;

	/* lengths are encoded from 256 in huffman alphabet */
	i += 256;
	
	/* write length index */
	bit_stream_write_bits(bs_out, huff_table->codes[i], huff_table->codes_len[i], BIT_ORDER_MSB);

	/* write length extra bits */
	deflate_huffman_encode_length_extra_bits(bs_out, length);
}

/**
 * @brief Read a symbol.
 * 
 * @param bs_in		input bit stream
 * @param huff_table	huffman table
 * 
 * @return symbol
 */
static int __read_symbol(struct bit_stream *bs_in, struct huffman_table *huff_table)
{
	int code = 0, code_len = 0, i;

	for (;;) {
		/* read next bit */
		code |= bit_stream_read_bits(bs_in, 1, BIT_ORDER_MSB);
		code_len++;

		/* try to find code in huffman table */
		for (i = 0; i < huff_table->len; i++)
			if (huff_table->codes_len[i] == code_len && huff_table->codes[i] == code)
				return i;

		/* go to next bit */
		code <<= 1;
	}
}

/**
 * @brief Compress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param lz77_nodes 		LZ77 nodes
 * @param bs_out 		output bit stream
 */
void deflate_dyn_huffman_compress(struct lz77_node *lz77_nodes, struct bit_stream *bs_out)
{
	struct huffman_table table_lit, table_dist;
	struct lz77_node *node;

	/* build huffman tables */
	__build_tables(lz77_nodes, &table_lit, &table_dist);

	/* write huffman tables */
	__write_huffman_tables(bs_out, &table_lit, &table_dist);

	/* compress each lz77 nodes */
	for (node = lz77_nodes; node != NULL; node = node->next) {
		if (node->is_literal) {
			__write_literal(node->data.literal, &table_lit, bs_out);
		} else {
			__write_length(node->data.match.length, &table_lit, bs_out);
			__write_distance(node->data.match.distance, &table_dist, bs_out);
		}
	}

	/* write end of block */
	bit_stream_write_bits(bs_out, table_lit.codes[256], table_lit.codes_len[256], BIT_ORDER_MSB);
}

/**
 * @brief Uncompress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param bs_in 		input bit stream
 * @param buf_out 		output buffer
 *
 * @return number of bytes written to output buffer
 */
int deflate_dyn_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out)
{
	struct huffman_table table_lit, table_dist;
	int literal, length, distance, n, i;

	/* read huffman tables */
	__read_huffman_tables(bs_in, &table_lit, &table_dist);

	for (n = 0;;) {
		/* read next literal */
		literal = __read_symbol(bs_in, &table_lit);

		/* end of block */
		if (literal == 256)
			break;

		/* literal : just add it to output buffer */
		if (literal < 256) {
			buf_out[n++] = literal;
			continue;
		}

		/* decode lz77 length */
		length = deflate_huffman_decode_length(bs_in, literal - 257);

		/* decode lz77 distance */
		distance = deflate_huffman_decode_distance(bs_in, __read_symbol(bs_in, &table_dist));

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return n;
}
