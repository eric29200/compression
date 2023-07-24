#include <stdio.h>
#include <stdlib.h>

#include "deflate.h"
#include "lz77.h"
#include "no_compression.h"
#include "fix_huffman.h"
#include "dyn_huffman.h"
#include "../utils/bit_stream.h"
#include "../utils/mem.h"

/*
 * Deflate compression.
 */
void deflate_compress(FILE *fp_in, FILE *fp_out)
{
	struct bit_stream_t *bs_fix_huff, *bs_dyn_huff, *bs_no, *bs;
	struct lz77_node_t *lz77_nodes = NULL;
	char *block;
	int len;

	/* create bit streams */
	bs_fix_huff = bit_stream_create(DEFLATE_BLOCK_SIZE * 4);
	bs_dyn_huff = bit_stream_create(DEFLATE_BLOCK_SIZE * 4);
	bs_no = bit_stream_create(DEFLATE_BLOCK_SIZE * 4);

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
		fix_huffman_compress(lz77_nodes, feof(fp_in), bs_fix_huff);

		/* dynamic huffman compression */
		dyn_huffman_compress(lz77_nodes, feof(fp_in), bs_dyn_huff);

		/* choose compression method */
		if (len + 4 < bs_fix_huff->byte_offset && len + 4 < bs_dyn_huff->byte_offset) {
			no_compression_compress(block, len, feof(fp_in), bs_no);
			bs = bs_no;
		} else if (bs_fix_huff->byte_offset < bs_dyn_huff->byte_offset) {
			bs = bs_fix_huff;
		} else {
			bs = bs_dyn_huff;
		}

		/* flush bit stream */
		bit_stream_flush(bs, fp_out, feof(fp_in));

		/* reset fix huffman bit stream (keep first byte) */
		bs_fix_huff->buf[0] = bs->buf[0];
		bs_fix_huff->byte_offset = 0;
		bs_fix_huff->bit_offset = bs->bit_offset;

		/* reset dynamic huffman bit stream (keep first byte) */
		bs_dyn_huff->buf[0] = bs->buf[0];
		bs_dyn_huff->byte_offset = 0;
		bs_dyn_huff->bit_offset = bs->bit_offset;

		/* reset no compression bit stream (keep first byte) */
		bs_no->buf[0] = bs->buf[0];
		bs_no->byte_offset = 0;
		bs_no->bit_offset = bs->bit_offset;

		/* free lz77 nodes */
		lz77_free_nodes(lz77_nodes);
	}

	/* free bit streams */
	bit_stream_free(bs_fix_huff);
	bit_stream_free(bs_dyn_huff);
	bit_stream_free(bs_no);
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
			case 0:
				len = no_compression_uncompress(bs, buf);
				break;
			case 1:
				len = fix_huffman_uncompress(bs, buf);
				break;
			case 2:
				len = dyn_huffman_uncompress(bs, buf);
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
