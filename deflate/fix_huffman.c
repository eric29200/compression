#include <stdio.h>
#include <string.h>

#include "fix_huffman.h"
#include "huffman.h"
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
 * @brief Build default huffman tables.
 * 
 * @param table_lit 		literals huffman table
 * @param table_dist 		distances huffman table
 */
void __build_default_tables(struct huffman_table *table_lit, struct huffman_table *table_dist)
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
 * @brief Compress lz77 nodes with fix huffman alphabet.
 * 
 * @param lz77_nodes 	LZ77 nodes
 * @param bs_out 	output bit stream
 */
void deflate_fix_huffman_compress(struct lz77_node *lz77_nodes, struct bit_stream *bs_out)
{
	struct huffman_table table_lit, table_dist;
	struct lz77_node *node;

	/* build huffman table */
	__build_default_tables(&table_lit, &table_dist);

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
	bit_stream_write_bits(bs_out, 0, 7, BIT_ORDER_MSB);
}

/**
 * @brief Uncompress LZ77 nodes with fix huffman codes.
 * 
 * @param bs_in 	input bit stream
 * @param buf_out 	output buffer
 * 
 * @return number of bytes written in output buffer
 */
int deflate_fix_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out)
{
	struct huffman_table table_lit, table_dist;
	int literal, length, distance, n, i;

	/* build default tables */
	__build_default_tables(&table_lit, &table_dist);

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