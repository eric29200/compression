#include <stdio.h>

#include "no_compression.h"

/**
 * @brief Compress a block.
 * 
 * @param block 	input block
 * @param len 		block length
 * @param bs_out 	output bit stream
 */
void deflate_no_compression_compress(uint8_t *block, uint16_t len, struct bit_stream *bs_out)
{
	uint32_t i;

	/* go to next byte */
	bit_stream_flush(bs_out);

	/* write length */
	bit_stream_write_bits(bs_out, len, 16, BIT_ORDER_LSB);

	/* write one's complement of length */
	bit_stream_write_bits(bs_out, ~len, 16, BIT_ORDER_LSB);

	/* write block */
	for (i = 0; i < len; i++)
		bit_stream_write_bits(bs_out, block[i], 8, BIT_ORDER_LSB);
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
	uint16_t len, i;

	/* go to next byte */
	bit_stream_flush(bs_in);

	/* read length */
	len = bit_stream_read_bits(bs_in, 16, BIT_ORDER_LSB);

	/* ignore one's complement of length */
	bit_stream_read_bits(bs_in, 16, BIT_ORDER_LSB);

	/* read block */
	for (i = 0; i < len; i++)
		buf_out[i] = bit_stream_read_bits(bs_in, 8, BIT_ORDER_LSB);

	return len;
}
