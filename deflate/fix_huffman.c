#include <stdio.h>
#include <string.h>

#include "fix_huffman.h"
#include "huffman.h"

/*
 * Write a literal.
 */
static void __write_literal(unsigned char literal, struct bit_stream_t *bs_out)
{
	unsigned int value = 0;

	if (literal < 144) {
		value = 0x30 + literal;
		bit_stream_write_bits(bs_out, value, 8);
	} else {
		value = 0x190 + literal - 144;
		bit_stream_write_bits(bs_out, value, 9);
	}
}

/*
 * Write a distance.
 */
void __write_distance(int distance, struct bit_stream_t *bs_out)
{
	int i;

	/* write distance index */
	i = huff_distance_index(distance);
	bit_stream_write_bits(bs_out, i, 5);

	/* write distance extra bits */
	huff_encode_distance_extra_bits(distance, bs_out);
}

/*
 * Write a length.
 */
void __write_length(int length, struct bit_stream_t *bs_out)
{
	int value, i;

	/* write length index */
	i = huff_length_index(length) + 1;
	if (i < 24) {
		value = i;
		bit_stream_write_bits(bs_out, value, 7);
	} else {
		value = 0xC0 + i - 24;
		bit_stream_write_bits(bs_out, value, 8);
	}

	/* write length extra bits */
	huff_encode_length_extra_bits(length, bs_out);
}

/*
 * Read next literal character.
 */
static int __read_next_literal(struct bit_stream_t *bs_in)
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

/*
 * Compress lz77 nodes with fix huffman alphabet.
 */
void fix_huffman_compress(struct lz77_node_t *lz77_nodes, int last_block, struct bit_stream_t *bs_out)
{
	struct lz77_node_t *node;

	/* write block header */
	if (last_block)
		bit_stream_write_bits(bs_out, 5, 3);
	else
		bit_stream_write_bits(bs_out, 1, 3);

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
	bit_stream_write_bits(bs_out, 0, 7);
}

/*
 * Uncompress lz77 nodes with fix huffman codes.
 */
int fix_huffman_uncompress(struct bit_stream_t *bs_in, char *buf_out)
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
		length = huff_decode_length(literal - 257, bs_in);
		distance = huff_decode_distance(bit_stream_read_bits(bs_in, 5), bs_in);

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return n;
}
