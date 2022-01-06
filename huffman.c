#include <stdio.h>

#include "huffman.h"

/*
 * Fix huffman lengths.
 */
static int huff_lengths[29] = {
  3,   4,  5,   6,   7,   8,   9,  10,  11, 13,
  15, 17, 19,  23,  27,  31,  35,  43,  51, 59,
  67, 83, 99, 115, 131, 163, 195, 227, 258
};

/*
 * Fix huffman lengths extra bits.
 */
static int huff_lengths_extra_bits[29] = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
  1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
  4, 4, 4, 4, 5, 5, 5, 5, 0
};

/*
 * Fix huffman distances.
 */
static int huff_distances[30] = {
     1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
    33,   49,   65,   97,  129,  193,  257,   385,   513,   769,
  1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

/*
 * Fix huffman distances extra bits.
 */
static int huff_distances_extra_bits[30] = {
  0, 0,  0,  0,  1,  1,  2,  2,  3,  3,
  4, 4,  5,  5,  6,  6,  7,  7,  8,  8,
  9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

/*
 * Get huffman distance code.
 */
int huff_distance_code(int distance)
{
  int i;

  for (i = 1; i < 30; i++)
    if (distance < huff_distances[i])
      break;

  return i - 1;
}
/*
 * Decode a distance literal.
 */
int huff_decode_distance(int literal, struct bit_stream_t *bs_in)
{
  return huff_distances[literal] + bit_stream_read_bits(bs_in, huff_distances_extra_bits[literal]);
}

/*
 * Encode and write a distance.
 */
void huff_encode_distance(int distance, struct bit_stream_t *bs_out)
{
  int code;

  code = huff_distance_code(distance);
  bit_stream_write_bits(bs_out, code, 5);
  bit_stream_write_bits(bs_out, distance - huff_distances[code], huff_distances_extra_bits[code]);
}

/*
 * Decode a length literal.
 */
int huff_decode_length(int literal, struct bit_stream_t *bs_in)
{
  return huff_lengths[literal - 257] + bit_stream_read_bits(bs_in, huff_lengths_extra_bits[literal - 257]);
}

/*
 * Get huffman length code.
 */
int huff_length_code(int length)
{
  int i;

  for (i = 1; i < 29; i++)
    if (length < huff_lengths[i])
      break;

  return i - 1;
}

/*
 * Encode and write a length.
 */
void huff_encode_length(int length, struct bit_stream_t *bs_out)
{
  int value, code;

  code = huff_length_code(length) + 1;

  if (code < 24) {
    value = code;
    bit_stream_write_bits(bs_out, value, 7);
  } else {
    value = 0xC0 + code - 24;
    bit_stream_write_bits(bs_out, value, 8);
  }

  bit_stream_write_bits(bs_out, length - huff_lengths[code - 1], huff_lengths_extra_bits[code - 1]);
}
