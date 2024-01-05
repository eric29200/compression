#include <stdio.h>

#include "no_compression.h"

/**
 * @brief Compress a block.
 * 
 * @param block 	input block
 * @param len 		block length
 * @param last_block 	is this last block ?
 * @param bs_out 	output bit stream
 */
void deflate_no_compression_compress(uint8_t *block, int len, int last_block, struct bit_stream *bs_out)
{
	int i;

	/* write block header */
	if (last_block)
		bit_stream_write_bits(bs_out, 4, 3, 0);
	else
		bit_stream_write_bits(bs_out, 0, 3, 0);

	/* write length */
	bit_stream_write_bits(bs_out, len & 0xFFFF, 16, 0);

	/* write block */
	for (i = 0; i < len; i++)
		bit_stream_write_bits(bs_out, block[i], 8, 0);
}

/**
 * @brief Uncompress a block.
 * 
 * @param bs_in 	input bit stream
 * @param buf_out 	output buffer
 * 
 * @return block length
 */
int deflate_no_compression_uncompress(struct bit_stream *bs_in, uint8_t *buf_out)
{
	int len, i;

	/* read length */
	len = bit_stream_read_bits(bs_in, 16);

	/* read block */
	for (i = 0; i < len; i++)
		buf_out[i] = bit_stream_read_bits(bs_in, 8);

	return len;
}
