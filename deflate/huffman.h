#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include "../utils/bit_stream.h"

#define HUFF_NB_LENGTH_CODES          29
#define HUFF_NB_DISTANCE_CODES        30

int huff_distance_index(int distance);
int huff_length_index(int length);
int huff_decode_distance(int index, struct bit_stream_t *bs_in);
int huff_decode_length(int index, struct bit_stream_t *bs_in);
void huff_encode_distance_extra_bits(int distance, struct bit_stream_t *bs_out);
void huff_encode_length_extra_bits(int length, struct bit_stream_t *bs_out);

#endif
