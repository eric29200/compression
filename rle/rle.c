/*
 * Run-Length Encoding algorithm = lossless data compression algorithm.
 * Redundant following characters are stored as (nr of occurences ; value).
 */
#include "rle.h"

#include "../utils/mem.h"

#define GROW_SIZE		64

/**
 * @brief Grow output buffer if needed.
 * 
 * @param dst 			output buffer
 * @param buf_out 		current position in output buffer
 * @param dst_capacity 		output buffer capacity
 * @param size_needed		size needed
 */
static void __grow_buffer(uint8_t **dst, uint8_t **buf_out, uint32_t *dst_capacity, uint32_t size_needed)
{
	uint32_t pos;

	/* no need to grower buffer */
	if ((uint32_t) (*buf_out - *dst + size_needed) <= *dst_capacity)
		return;

	/* remember position */
	pos = *buf_out - *dst;

	/* reallocate destination buffer */
	*dst_capacity += GROW_SIZE;
	*dst = xrealloc(*dst, *dst_capacity);
	
	/* set new position */
	*buf_out = *dst + pos;
}

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
	uint32_t i, j, dst_capacity;
	uint8_t *dst, *buf_out;

	/* allocate output buffer */
	dst_capacity = src_len;
	dst = buf_out = (uint8_t *) xmalloc(dst_capacity);

	/* write uncompressed length */
	*((uint32_t *) buf_out) = src_len;
	buf_out += sizeof(uint32_t);

	/* compress */
	for (i = 0; i < src_len;) {
		/* look for same following characters */
		for (j = 0; j < UINT8_MAX && i + j < src_len && src[i] == src[i + j]; j++);

		/* grow buffer if needed */
		__grow_buffer(&dst, &buf_out, &dst_capacity, sizeof(uint32_t) + sizeof(uint8_t));

		/* write number of occurences */
		*((uint8_t *) buf_out) = (uint8_t) j;
		buf_out += sizeof(uint8_t);

		/* write character */
		*buf_out++ = src[i];

		/* go to next character */
		i += j;
	}

	/* compute destination length */
	*dst_len = buf_out - dst;

	return dst;
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
	*dst_len = *((uint32_t *) buf_in);
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