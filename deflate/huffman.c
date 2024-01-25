#include <stdio.h>

#include "huffman.h"
#include "fix_huffman.h"
#include "dyn_huffman.h"

/**
 * @brief Fix huffman lengths.
 */
static int huffman_lengths[NR_LENGTHS] = {
	3,	 4,	5,	  6,	  7,	  8,	  9,	 10,	 11, 	13,
	15, 	17, 	19,	 23,	 27,	 31,	 35,	 43,	 51, 	59,
	67, 	83, 	99, 	115, 	131, 	163, 	195, 	227, 	258
};

/**
 * @brief Fix huffman lengths extra bits.
 */
static int huffman_lengths_extra_bits[NR_LENGTHS] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
	4, 4, 4, 4, 5, 5, 5, 5, 0
};

/**
 * @brief Fix huffman distances.
 */
static int huffman_distances[NR_DISTANCES] = {
	   1,	   2,	   3,	   4,	   5,	   7,	   9,	   13,	   17,	   25,
	  33,	  49,	  65,	  97,	 129,	 193,	 257,	  385,	  513,	  769,
	1025, 	1537, 	2049, 	3073, 	4097, 	6145, 	8193, 	12289, 	16385, 	24577
};

/**
 * @brief Fix huffman distances extra bits.
 */
static int huffman_distances_extra_bits[NR_DISTANCES] = {
	0, 	0,	 0,	 0,	 1,	 1,	 2,	 2,	 3,	 3,
	4, 	4,	 5,	 5,	 6,	 6,	 7,	 7,	 8,	 8,
	9, 	9, 	10, 	10, 	11, 	11, 	12, 	12, 	13, 	13
};

/**
 * @brief Get huffman distance index.
 * 
 * @param distance	distance
 */
int deflate_huffman_distance_index(int distance)
{
	int i;

	for (i = 1; i < NR_DISTANCES; i++)
		if (distance < huffman_distances[i])
			break;

	return i - 1;
}

/**
 * @brief Get huffman length index.
 * 
 * @param length	length
 * 
 * @return index
 */
int deflate_huffman_length_index(int length)
{
	int i;

	for (i = 1; i < NR_LENGTHS; i++)
		if (length < huffman_lengths[i])
			break;

	return i - 1;
}

/**
 * @brief Decode a distance literal.
 * 
 * @param bs_in		input bit stream
 * @param index		index
 * 
 * @return distance
 */
static int __decode_distance(struct bit_stream *bs_in, int index)
{
	return huffman_distances[index] + bit_stream_read_bits(bs_in, huffman_distances_extra_bits[index], BIT_ORDER_LSB);
}

/**
 * @brief Decode a length literal.
 *
 * @param bs_in		input bit stream
 * @param index		index
 * 
 * @return length
 */
static int __decode_length(struct bit_stream *bs_in, int index)
{
	return huffman_lengths[index] + bit_stream_read_bits(bs_in, huffman_lengths_extra_bits[index], BIT_ORDER_LSB);
}

/**
 * @brief Write a literal.
 * 
 * @param literal 		literal
 * @param table			huffman table
 * @param bs_out 		output bit stream
 */
static void __write_literal(uint8_t literal, struct huffman_table *table, struct bit_stream *bs_out)
{
	bit_stream_write_bits(bs_out, table->codes[literal], table->codes_len[literal], BIT_ORDER_MSB);
}

/**
 * @brief Write a distance.
 * 
 * @param distance 		distance
 * @param table			huffman table
 * @param bs_out 		output bit stream
 */
static void __write_distance(int distance, struct huffman_table *table, struct bit_stream *bs_out)
{
	/* get distance index */
	int i = deflate_huffman_distance_index(distance);
	
	/* write distance index */
	bit_stream_write_bits(bs_out, table->codes[i], table->codes_len[i], BIT_ORDER_MSB);

	/* write distance extra bits */
	bit_stream_write_bits(bs_out, distance - huffman_distances[i], huffman_distances_extra_bits[i], BIT_ORDER_LSB);
}

/**
 * @brief Write a length.
 * 
 * @param length 		length
 * @param table		huffman table
 * @param bs_out 		output bit stream
 */
static void __write_length(int length, struct huffman_table *table, struct bit_stream *bs_out)
{
	/* get length index */
	int i = deflate_huffman_length_index(length);

	/* write length index */
	bit_stream_write_bits(bs_out, table->codes[i + 257], table->codes_len[i + 257], BIT_ORDER_MSB);

	/* write length extra bits */
	bit_stream_write_bits(bs_out, length - huffman_lengths[i], huffman_lengths_extra_bits[i], BIT_ORDER_LSB);
}

/**
 * @brief Compress LZ77 nodes with huffman alphabet.
 * 
 * @param lz77_nodes 		LZ77 nodes
 * @param bs_out 		output bit stream
 * @param dynamic		use dynamic alphabet ?
 */
void deflate_huffman_compress(struct lz77_node *lz77_nodes, struct bit_stream *bs_out, int dynamic)
{
	struct huffman_table table_lit, table_dist;
	struct lz77_node *node;

	/* build huffman tables */
	if (dynamic)
		deflate_huffman_build_dynamic_tables(lz77_nodes, &table_lit, &table_dist);
	else
		deflate_huffman_build_fix_tables(&table_lit, &table_dist);

	/* write huffman tables */
	if (dynamic)
		deflate_huffman_write_tables(bs_out, &table_lit, &table_dist);

	/* compress each lz77 node */
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

	/* free huffman tables */
	huffman_table_free(&table_lit);
	huffman_table_free(&table_dist);
}

/**
 * @brief Uncompress LZ77 nodes with huffman alphabet.
 * 
 * @param bs_in 		input bit stream
 * @param buf_out 		output buffer
 * @param dynamic		use dynamic alphabet ?
 *
 * @return number of bytes written to output buffer
 */
int deflate_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out, int dynamic)
{
	struct huffman_table table_lit, table_dist;
	int literal, length, distance, n, i;

	/* build huffman tables */
	if (dynamic)
		deflate_huffman_read_tables(bs_in, &table_lit, &table_dist);
	else
		deflate_huffman_build_fix_tables(&table_lit, &table_dist);

	/* uncompress */
	for (n = 0;;) {
		/* read next literal */
		literal = huffman_table_read_symbol(bs_in, &table_lit);

		/* end of block */
		if (literal == 256)
			break;

		/* literal : just add it to output buffer */
		if (literal < 256) {
			buf_out[n++] = literal;
			continue;
		}

		/* decode lz77 length */
		length = __decode_length(bs_in, literal - 257);

		/* decode lz77 distance */
		distance = __decode_distance(bs_in, huffman_table_read_symbol(bs_in, &table_dist));

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return n;
}
