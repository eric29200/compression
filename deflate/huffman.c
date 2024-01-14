#include <stdio.h>

#include "huffman.h"

/**
 * @brief Fix huffman lengths.
 */
static int huff_lengths[DEFLATE_HUFFMAN_NR_LENGTH_CODES] = {
	3,	 4,	5,	  6,	  7,	  8,	  9,	 10,	 11, 	13,
	15, 	17, 	19,	 23,	 27,	 31,	 35,	 43,	 51, 	59,
	67, 	83, 	99, 	115, 	131, 	163, 	195, 	227, 	258
};

/**
 * @brief Fix huffman lengths extra bits.
 */
static int huff_lengths_extra_bits[DEFLATE_HUFFMAN_NR_LENGTH_CODES] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
	4, 4, 4, 4, 5, 5, 5, 5, 0
};

/**
 * @brief Fix huffman distances.
 */
static int huff_distances[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = {
	   1,	   2,	   3,	   4,	   5,	   7,	   9,	   13,	   17,	   25,
	  33,	  49,	  65,	  97,	 129,	 193,	 257,	  385,	  513,	  769,
	1025, 	1537, 	2049, 	3073, 	4097, 	6145, 	8193, 	12289, 	16385, 	24577
};

/**
 * @brief Fix huffman distances extra bits.
 */
static int huff_distances_extra_bits[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = {
	0, 	0,	 0,	 0,	 1,	 1,	 2,	 2,	 3,	 3,
	4, 	4,	 5,	 5,	 6,	 6,	 7,	 7,	 8,	 8,
	9, 	9, 	10, 	10, 	11, 	11, 	12, 	12, 	13, 	13
};

/**
 * @brief Get huffman distance index.
 * 
 * @param distance	distance
 */
int deflate_fix_huffman_distance_index(int distance)
{
	int i;

	for (i = 1; i < DEFLATE_HUFFMAN_NR_DISTANCE_CODES; i++)
		if (distance < huff_distances[i])
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
int deflate_fix_huffman_length_index(int length)
{
	int i;

	for (i = 1; i < DEFLATE_HUFFMAN_NR_LENGTH_CODES; i++)
		if (length < huff_lengths[i])
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
int deflate_fix_huffman_decode_distance(struct bit_stream *bs_in, int index)
{
	return huff_distances[index] + bit_stream_read_bits(bs_in, huff_distances_extra_bits[index]);
}

/**
 * @brief Decode a length literal.
 *
 * @param bs_in		input bit stream
 * @param index		index
 * 
 * @return length
 */
int deflate_fix_huffman_decode_length(struct bit_stream *bs_in, int index)
{
	return huff_lengths[index] + bit_stream_read_bits(bs_in, huff_lengths_extra_bits[index]);
}

/**
 * @brief Encode and write distance extra bits.
 *
 * @param bs_out	output bit stream
 * @param distance	distance
 */
void deflate_fix_huffman_encode_distance_extra_bits(struct bit_stream *bs_out, int distance)
{
	int i;

	i = deflate_fix_huffman_distance_index(distance);
	bit_stream_write_bits(bs_out, distance - huff_distances[i], huff_distances_extra_bits[i]);
}

/**
 * @brief Encode and write length extra bits.
 * 
 * @param bs_out	output bit stream
 * @param length	length
 */
void deflate_fix_huffman_encode_length_extra_bits(struct bit_stream *bs_out, int length)
{
	int i;

	i = deflate_fix_huffman_length_index(length) + 1;
	bit_stream_write_bits(bs_out, length - huff_lengths[i - 1], huff_lengths_extra_bits[i - 1]);
}
