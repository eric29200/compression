#include <stdio.h>
#include <string.h>

#include "fix_huffman.h"
#include "huffman.h"

/**
 * @brief Write a literal.
 * 
 * @param literal 	literal
 * @param bs_out 	output bit stream
 */
static void __write_literal(uint8_t literal, struct bit_stream *bs_out)
{
	unsigned int value = 0;

	if (literal < 144) {
		value = 0x30 + literal;
		bit_stream_write_bits(bs_out, value, 8, 0);
	} else {
		value = 0x190 + literal - 144;
		bit_stream_write_bits(bs_out, value, 9, 0);
	}
}

/**
 * @brief Write a distance.
 * 
 * @param distance 	distance
 * @param bs_out 	output bit stream
 */
static void __write_distance(int distance, struct bit_stream *bs_out)
{
	int i;

	/* write distance index */
	i = deflate_fix_huffman_distance_index(distance);
	bit_stream_write_bits(bs_out, i, 5, 0);

	/* write distance extra bits */
	deflate_fix_huffman_encode_distance_extra_bits(bs_out, distance);
}

/**
 * @brief Write a length.
 * 
 * @param length 	length
 * @param bs_out 	output bit stream
 */
static void __write_length(int length, struct bit_stream *bs_out)
{
	int value, i;

	/* write length index */
	i = deflate_fix_huffman_length_index(length) + 1;
	if (i < 24) {
		value = i;
		bit_stream_write_bits(bs_out, value, 7, 0);
	} else {
		value = 0xC0 + i - 24;
		bit_stream_write_bits(bs_out, value, 8, 0);
	}

	/* write length extra bits */
	deflate_fix_huffman_encode_length_extra_bits(bs_out, length);
}

/**
 * @brief Read next literal character.
 * 
 * @param bs_int	input bit stream
 * 
 * @return next literal character
 */
static int __read_next_literal(struct bit_stream *bs_in)
{
	unsigned int value;

	/* first read 7 bits */
	value = bit_stream_read_bits(bs_in, 7);

	if (value <= 23)
		return value + 256;

	/* no match, read next bit (8 bits) */
	value <<= 1;
	value |= bit_stream_read_bits(bs_in, 1);

	if (value >= 48 && value <= 191)
		return value - 48;

	if (value >= 192 && value <= 199)
		return value + 88;

	/* still no match, read next bit (9 bits) */
	value <<= 1;
	value |= bit_stream_read_bits(bs_in, 1);

	if (value >= 400 && value <= 511)
		return value - 256;

	return -1;
}

/**
 * @brief Compress lz77 nodes with fix huffman alphabet.
 * 
 * @param lz77_nodes 	LZ77 nodes
 * @param last_block 	is this last block ?
 * @param bs_out 	output bit stream
 */
void deflate_fix_huffman_compress(struct lz77_node *lz77_nodes, int last_block, struct bit_stream *bs_out)
{
	struct lz77_node *node;

	/* write block header */
	if (last_block)
		bit_stream_write_bits(bs_out, 5, 3, 0);
	else
		bit_stream_write_bits(bs_out, 1, 3, 0);

	/* compress each lz77 nodes */
	for (node = lz77_nodes; node != NULL; node = node->next) {
		/* write literal or <length, distance> */
		if (node->is_literal) {
			__write_literal(node->data.literal, bs_out);
		} else {
			__write_length(node->data.match.length, bs_out);
			__write_distance(node->data.match.distance, bs_out);
		}
	}

	/* write end of block */
	bit_stream_write_bits(bs_out, 0, 7, 0);
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
	int literal, length, distance, n, i;

	for (n = 0;;) {
		/* read next literal */
		literal = __read_next_literal(bs_in);

		/* end of block */
		if (literal == 256)
			break;

		/* literal : just add it to output buffer */
		if (literal < 256) {
			buf_out[n++] = literal;
			continue;
		}

		/* decode lz77 length and distance */
		length = deflate_fix_huffman_decode_length(bs_in, literal - 257);
		distance = deflate_fix_huffman_decode_distance(bs_in, bit_stream_read_bits(bs_in, 5));

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return n;
}
