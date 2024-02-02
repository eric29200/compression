/*
 * Run-Length Encoding algorithm = lossless data compression algorithm.
 * Redundant following characters are stored as (nr of occurences ; value).
 */
#include <endian.h>

#include "rle.h"
#include "../utils/mem.h"
#include "../utils/byte_stream.h"

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
	struct byte_stream bs_out = { 0 };
	uint32_t i, j;

	/* write uncompressed length */
	byte_stream_write_u32(&bs_out, htole32(src_len));

	/* compress */
	for (i = 0; i < src_len;) {
		/* look for same following characters */
		for (j = 0; j < UINT8_MAX && i + j < src_len && src[i] == src[i + j]; j++);

		/* write number of occurences */
		byte_stream_write_u8(&bs_out, j);

		/* write character */
		byte_stream_write_u8(&bs_out, src[i]);

		/* go to next character */
		i += j;
	}

	/* set destination length */
	*dst_len = bs_out.size;

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
	uint8_t *dst, *buf_in, *buf_out, nr, c;
	uint32_t i;

	/* set input buffer */
	buf_in = src;

	/* read uncompressed length */
	*dst_len = le32toh(*((uint32_t *) buf_in));
	buf_in += sizeof(uint32_t);

	/* allocate output buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* uncompress */
	while (buf_in < src + src_len) {
		/* read number of occurences and character */
		nr = *buf_in++;
		c = *buf_in++;

		/* write charaters to output */
		for (i = 0; i < nr; i++)
			*buf_out++ = c;
	}

	return dst;
}