/*
 * LZSS algorithm = lossless data compression algorithm.
 * Improved version of LZ77 = emit a match (offset,length) only if it improves size.
 */
#include <string.h>
#include <endian.h>

#include "lzss.h"
#include "../utils/bit_stream.h"
#include "../utils/mem.h"

#define MATCH_MIN_LEN		3
#define WINDOW_SIZE		255
#define MIN(x, y)		((x) < (y) ? (x) : (y))

/**
 * @brief LZSS match.
 */
struct lzss_match {
	uint8_t		off;		/* offset from current position */
	uint8_t		len;		/* match length */
};

/**
 * @brief Find a matching pattern inside a window.
 * 
 * @param window 	window (= look aside buffer)
 * @param buf 		buffer to compress
 * @param len 		buffer length
 * @param node		output node
 */
static void __lzss_match(uint8_t *window, uint8_t *buf, uint32_t len, struct lzss_match *match)
{
	uint32_t i, j, max_match_len;

	/* reset lzss match */
	match->off = 0;
	match->len = 0;

	for (i = 0; i < WINDOW_SIZE; i++) {
		/* compute max match length */
		max_match_len = MIN(WINDOW_SIZE - i, len);

		/* compute match */
		for (j = 0; j < max_match_len && window[i + j] == buf[j]; j++);

		/* update best match */
		if (j > match->len) {
			match->off = WINDOW_SIZE - i;
			match->len = j;
		}
	}
}

/**
 * @brief Compress a buffer with LZSS algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lzss_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct bit_stream bs_out = { 0 };
	uint8_t *window, *buf_in;
	uint32_t window_size, i;
	struct lzss_match match;

	/* write uncompressed length first */
	bit_stream_write_bits(&bs_out, htole32(src_len), 32, BIT_ORDER_MSB);

	/* set input buffer and initial window */
	buf_in = window = src;

	/* copy first window to destination */
	window_size = src_len < WINDOW_SIZE ? src_len : WINDOW_SIZE;
	for (i = 0; i < window_size; i++)
	 	bit_stream_write_bits(&bs_out, *buf_in++, 8, BIT_ORDER_MSB);

	/* compress nodes */
	while (buf_in < src + src_len) {
		/* find best match */
		__lzss_match(window, buf_in, src + src_len - buf_in, &match);

		/* write match */
		if (match.len >= MATCH_MIN_LEN) {
			bit_stream_write_bits(&bs_out, 1, 1, BIT_ORDER_MSB);
			bit_stream_write_bits(&bs_out, match.off, 8, BIT_ORDER_MSB);
			bit_stream_write_bits(&bs_out, match.len, 8, BIT_ORDER_MSB);

			/* update window and buffer */
			window += match.len;
			buf_in += match.len;
			continue;
		}

		/* else write literal */
		bit_stream_write_bits(&bs_out, 0, 1, BIT_ORDER_MSB);
		bit_stream_write_bits(&bs_out, *buf_in, 8, BIT_ORDER_MSB);

		/* update window and buffer */
		window++;
		buf_in++;
	}

	/* set destination length */
	bit_stream_flush(&bs_out);
	*dst_len = bs_out.byte_offset;

	return bs_out.buf;
}

/**
 * @brief Uncompress a buffer with LZSS algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lzss_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct bit_stream bs_in = { 0 };
	uint8_t *dst, *buf_out, type;
	struct lzss_match match;
	uint32_t window_size, i;
	
	/* set input bit stream */
	bs_in.buf = src;
	bs_in.capacity = src_len;

	/* read uncompressed length first */
	*dst_len = le32toh(bit_stream_read_bits(&bs_in, 32, BIT_ORDER_MSB));
	
	/* allocate destination buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* copy first window to destination */
	window_size = src_len < WINDOW_SIZE ? src_len : WINDOW_SIZE;
	for (i = 0; i < window_size; i++)
		*buf_out++ = bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB);

	/* uncompress nodes */
	while (buf_out < dst + *dst_len) {
		/* read type (match or literal) */
		type = bit_stream_read_bits(&bs_in, 1, BIT_ORDER_MSB);

		/* decode match */
		if (type) {
			match.off = bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB);
			match.len = bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB);
			memcpy(buf_out, buf_out - match.off, match.len);
			buf_out += match.len;
			continue;
		}

		/* else decode literal */
		*buf_out++ = bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB);
	}

	return dst;
}