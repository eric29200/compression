#include <stdio.h>

#include "fix_huffman.h"

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
static void __write_distance(int distance, struct bit_stream_t *bs_out)
{
  int i;

  for (i = 1; i < 30; i++)
    if (distance < huff_distances[i])
      break;

  bit_stream_write_bits(bs_out, i - 1, 5);
  bit_stream_write_bits(bs_out, distance - huff_distances[i - 1], huff_distances_extra_bits[i - 1]);
}

/*
 * Write a length.
 */
static void __write_length(int length, struct bit_stream_t *bs_out)
{
  int value, i;

  for (i = 1; i < 29; i++)
    if (length < huff_lengths[i])
      break;

  if (i - 1 < 24) {
    value = i - 1;
    bit_stream_write_bits(bs_out, value, 7);
  } else {
    value = 0xC0 + i - 1 - 24;
    bit_stream_write_bits(bs_out, value, 8);
  }

  bit_stream_write_bits(bs_out, length - huff_lengths[i - 1], huff_lengths_extra_bits[i - 1]);
}

/*
 * Compress lz77 nodes with fix huffman alphabet.
 */
void fix_huffman_compress(struct lz77_node_t *lz77_nodes, int last_block, struct bit_stream_t *bs_out)
{
  struct lz77_node_t *node;

  /*
   * write block header :
   * - 1st bit :
   *   - 1 = last block
   *   - 0 = non last block
   * - 2nd and 3rd bits :
   *   - 00 = no compression
   *   - 01 = compressed with fix huffman codes
   *   - 10 = compressed with dynamic huffman codes
   *   - 11 = reserved
   */
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
