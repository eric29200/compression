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
void deflate_no_compression_compress(uint8_t *block, uint32_t len, int last_block, struct bit_stream *bs_out)
{
	uint32_t i;

	/* write block header (final block + compression method) */
	bit_stream_write_bits(bs_out, last_block, 1);
	bit_stream_write_bits(bs_out, 0, 2);

	/* write length */
	bit_stream_write_bits(bs_out, len, 32);

	/* write block */
	for (i = 0; i < len; i++)
		bit_stream_write_bits(bs_out, block[i], 8);
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
	uint32_t len, i;

	/* read length */
	len = bit_stream_read_bits(bs_in, 32);

	/* read block */
	for (i = 0; i < len; i++)
		buf_out[i] = bit_stream_read_bits(bs_in, 8);

	return len;
}
