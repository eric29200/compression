#include <stdio.h>
#include <stdlib.h>

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
  struct lz77_node_t *lz77_nodes;
  struct bit_stream_t *bs;
  char *block;
  int len;

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

    /* flush bit stream */
    bit_stream_flush(bs, fp_out, feof(fp_in));

    /* free lz77 nodes */
    lz77_free_nodes(lz77_nodes);
  }

  /* free bit stream */
  bit_stream_free(bs);
}

/*
 * Deflate uncompression.
 */
void deflate_uncompress(FILE *fp_in, FILE *fp_out)
{
  int last_block, type, len;
  struct bit_stream_t *bs;
  char *buf;

  /* create buffer */
  buf = (char *) xmalloc(DEFLATE_BLOCK_SIZE);

  /* create bit stream */
  bs = bit_stream_create(DEFLATE_BLOCK_SIZE * 4);

  /* read some data */
  bit_stream_read(bs, fp_in, 1);

  /* uncompress */
  for (last_block = 0; last_block == 0;) {
    /* get block header */
    last_block = bit_stream_read_bit(bs);
    type = bit_stream_read_bits(bs, 2);

    /* handle compression type */
    switch (type) {
      case 1:
        len = fix_huffman_uncompress(bs, buf);
        break;
      default:
        fprintf(stderr, "Unknown compression type\n");
        goto out;
    }

    /* write buffer */
    if (len > 0)
      fwrite(buf, sizeof(char), len, fp_out);

    /* read next data */
    bit_stream_read(bs, fp_in, 0);
  }

out:
  /* free bit stream */
  bit_stream_free(bs);

  /* free buffer */
  free(buf);
}
