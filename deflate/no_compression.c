#include <stdio.h>

#include "no_compression.h"

/*
 * Compress a block (actually no compression).
 */
void no_compression_compress(unsigned char *block, int len, int last_block, struct bit_stream_t *bs_out)
{
	int i;

	/* write block header */
	if (last_block)
		bit_stream_write_bits(bs_out, 4, 3);
	else
		bit_stream_write_bits(bs_out, 0, 3);

	/* write length */
	bit_stream_write_bits(bs_out, len & 0xFFFF, 16);

	/* write block */
	for (i = 0; i < len; i++)
		bit_stream_write_bits(bs_out, block[i], 8);
}

/*
 * Uncompress a block.
 */
int no_compression_uncompress(struct bit_stream_t *bs_in, unsigned char *buf_out)
{
	int len, i;

	/* read length */
	len = bit_stream_read_bits(bs_in, 16);

	/* read block */
	for (i = 0; i < len; i++)
		buf_out[i] = bit_stream_read_bits(bs_in, 8);

	return len;
}
