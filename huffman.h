#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include "bit_stream.h"

int huff_distance_code(int distance);
int huff_decode_distance(int literal, struct bit_stream_t *bs_in);
void huff_encode_distance(int distance, struct bit_stream_t *bs_out);
int huff_length_code(int length);
int huff_decode_length(int literal, struct bit_stream_t *bs_in);
void huff_encode_length(int length, struct bit_stream_t *bs_out);

#endif
