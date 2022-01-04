#include <stdio.h>

#include "deflate.h"
#include "lz77.h"
#include "fix_huffman.h"
#include "bit_stream.h"
#include "mem.h"

/*
 * Deflate compression.
 */
void deflate_compress(FILE *fp_in, FILE *fp_out)
{
  int len, nb_last_bits, last_bits;
  struct lz77_node_t *lz77_nodes;
  struct bit_stream_t *bs;
  char *block;

  /* create bit stream */
  bs = bit_stream_create(DEFLATE_BLOCK_SIZE * 4);

  /* allocate block buffer */
  block = (char *) xmalloc(DEFLATE_BLOCK_SIZE);

  /* compress block by block */
  for (;;) {
    /* read next block */
    len = fread(block, 1, DEFLATE_BLOCK_SIZE, fp_in);
    if (len <= 0)
      break;

    /* LZ77 compression */
    lz77_nodes = lz77_compress(block, len);

    /* fix huffman compression */
    fix_huffman_compress(lz77_nodes, feof(fp_in), bs);

    /* write compressed data to output file */
    fwrite(bs->buf, sizeof(char), bs->byte_offset, fp_out);

    /* free lz77 nodes */
    lz77_free_nodes(lz77_nodes);

    /* copy remaining bits at the beginning of the stream */
    if (bs->bit_offset > 0)
      bs->buf[0] = bs->buf[bs->byte_offset];

    /* reset bit stream (do no reset bit offset !) */
    bs->byte_offset = 0;
  }

  /* write last bits */
  nb_last_bits = bs->bit_offset;
  if (nb_last_bits > 0) {
    bs->bit_offset = 0;
    last_bits = bit_stream_read_bits(bs, nb_last_bits);
    fwrite(&last_bits, sizeof(char), 1, fp_out);
  }

  /* free bit stream */
  bit_stream_free(bs);
}
