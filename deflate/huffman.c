#include <stdio.h>

#include "huffman.h"

/*
 * Fix huffman lengths.
 */
static int huff_lengths[HUFF_NB_LENGTH_CODES] = {
  3,   4,  5,   6,   7,   8,   9,  10,  11, 13,
  15, 17, 19,  23,  27,  31,  35,  43,  51, 59,
  67, 83, 99, 115, 131, 163, 195, 227, 258
};

/*
 * Fix huffman lengths extra bits.
 */
static int huff_lengths_extra_bits[HUFF_NB_LENGTH_CODES] = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
  1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
  4, 4, 4, 4, 5, 5, 5, 5, 0
};

/*
 * Fix huffman distances.
 */
static int huff_distances[HUFF_NB_DISTANCE_CODES] = {
     1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
    33,   49,   65,   97,  129,  193,  257,   385,   513,   769,
  1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

/*
 * Fix huffman distances extra bits.
 */
static int huff_distances_extra_bits[HUFF_NB_DISTANCE_CODES] = {
  0, 0,  0,  0,  1,  1,  2,  2,  3,  3,
  4, 4,  5,  5,  6,  6,  7,  7,  8,  8,
  9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

/*
 * Get huffman distance index.
 */
int huff_distance_index(int distance)
{
  int i;

  for (i = 1; i < HUFF_NB_DISTANCE_CODES; i++)
    if (distance < huff_distances[i])
      break;

  return i - 1;
}

/*
 * Get huffman length index.
 */
int huff_length_index(int length)
{
  int i;

  for (i = 1; i < HUFF_NB_LENGTH_CODES; i++)
    if (length < huff_lengths[i])
      break;

  return i - 1;
}

/*
 * Decode a distance literal.
 */
int huff_decode_distance(int index, struct bit_stream_t *bs_in)
{
  return huff_distances[index] + bit_stream_read_bits(bs_in, huff_distances_extra_bits[index]);
}

/*
 * Decode a length literal.
 */
int huff_decode_length(int index, struct bit_stream_t *bs_in)
{
  return huff_lengths[index] + bit_stream_read_bits(bs_in, huff_lengths_extra_bits[index]);
}

/*
 * Encode and write distance extra bits.
 */
void huff_encode_distance_extra_bits(int distance, struct bit_stream_t *bs_out)
{
  int i;

  i = huff_distance_index(distance);
  bit_stream_write_bits(bs_out, distance - huff_distances[i], huff_distances_extra_bits[i]);
}

/*
 * Encode and write length extra bits.
 */
void huff_encode_length_extra_bits(int length, struct bit_stream_t *bs_out)
{
  int i;

  i = huff_length_index(length) + 1;
  bit_stream_write_bits(bs_out, length - huff_lengths[i - 1], huff_lengths_extra_bits[i - 1]);
}
