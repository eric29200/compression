/*
 * Run-Length Encoding algorithm = lossless data compression algorithm.
 * Redundant following characters are stored as (nr of occurences ; value).
 */
#include <endian.h>

#include "rle.h"
#include "../utils/mem.h"
#include "../utils/bit_stream.h"

/**
 * @brief Compress a buffer with Run-Length Encoding algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *rle_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct bit_stream bs_out = { 0 };
	uint32_t i, j;

	/* write uncompressed length */
	bit_stream_write_bits(&bs_out, htole32(src_len), 32, BIT_ORDER_MSB);

	/* compress */
	for (i = 0; i < src_len;) {
		/* look for same following characters */
		for (j = 0; j < UINT8_MAX && i + j < src_len && src[i] == src[i + j]; j++);

		/* write compressed/uncompressed bit */
		bit_stream_write_bits(&bs_out, j > 1 ? 1 : 0, 1, BIT_ORDER_MSB);

		/* write number of occurences */
		if (j > 1)
			bit_stream_write_bits(&bs_out, j, 8, BIT_ORDER_MSB);

		/* write character */
		bit_stream_write_bits(&bs_out, src[i], 8, BIT_ORDER_MSB);

		/* go to next character */
		i += j;
	}

	/* flust last byte */
	bit_stream_flush(&bs_out);

	/* set destination length */
	*dst_len = bs_out.byte_offset;

	return bs_out.buf;
}

/**
 * @brief Uncompress a buffer with Run-Length Encoding algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *rle_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	uint8_t *dst, *buf_out, nr, c;
	struct bit_stream bs_in = { 0 };
	uint32_t i;

	/* set input bit stream */
	bs_in.buf = src;

	/* read uncompressed length */
	*dst_len = le32toh(bit_stream_read_bits(&bs_in, 32, BIT_ORDER_MSB));

	/* allocate output buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* uncompress */
	while (buf_out - dst < *dst_len && bs_in.byte_offset < src_len) {
		/* read compressed/uncompressed bit */
		nr = bit_stream_read_bits(&bs_in, 1, BIT_ORDER_MSB);

		/* read number of occurences and character */
		nr = nr ? bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB): 1;

		/* read character */
		c = bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB);

		/* write charaters to output */
		for (i = 0; i < nr; i++)
			*buf_out++ = c;
	}

	return dst;
}