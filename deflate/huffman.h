#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include "../utils/bit_stream.h"

#define DEFLATE_HUFFMAN_NR_LENGTH_CODES				29
#define DEFLATE_HUFFMAN_NR_DISTANCE_CODES			30

/**
 * @brief Get huffman distance index.
 * 
 * @param distance	distance
 */
int deflate_huffman_distance_index(int distance);

/**
 * @brief Get huffman length index.
 * 
 * @param length	length
 * 
 * @return index
 */
int deflate_huffman_length_index(int length);

/**
 * @brief Decode a distance literal.
 * 
 * @param bs_in		input bit stream
 * @param index		index
 * 
 * @return distance
 */
int deflate_huffman_decode_distance(struct bit_stream *bs_in, int index);

/**
 * @brief Decode a length literal.
 *
 * @param bs_in		input bit stream
 * @param index		index
 * 
 * @return length
 */
int deflate_huffman_decode_length(struct bit_stream *bs_in, int index);

/**
 * @brief Encode and write distance extra bits.
 *
 * @param bs_out	output bit stream
 * @param distance	distance
 */
void deflate_huffman_encode_distance_extra_bits(struct bit_stream *bs_out, int distance);

/**
 * @brief Encode and write length extra bits.
 * 
 * @param bs_out	output bit stream
 * @param length	length
 */
void deflate_huffman_encode_length_extra_bits(struct bit_stream *bs_out, int length);


#endif
