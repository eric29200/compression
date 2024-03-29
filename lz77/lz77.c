/*
 * LZ77 algorithm = lossless data compression algorithm.
 * This algorithm maintains a sliding window.
 * 1 - read first window of input buffer and write it exactly to the output buffer
 * 2 - try to find a matching pattern of next characher(s) in the window
 *	-> if it matches, write window reference (offset), pattern length and next character
 *	-> else write 0,0 and current character
 */
#include <string.h>
#include <endian.h>

#include "lz77.h"
#include "../utils/byte_stream.h"
#include "../utils/mem.h"

#define WINDOW_SIZE		255
#define MIN(x, y)		((x) < (y) ? (x) : (y))

/**
 * @brief LZ77 node.
 */
struct lz77_node {
	uint8_t		off;		/* offset from current position */
	uint8_t		len;		/* match length */
	uint8_t		literal;	/* next literal character */
};

/**
 * @brief Find a matching pattern inside a window.
 * 
 * @param window 	window (= look aside buffer)
 * @param buf 		buffer to compress
 * @param len 		buffer length
 * @param node		output node
 */
static void __lz77_match(uint8_t *window, uint8_t *buf, uint32_t len, struct lz77_node *node)
{
	uint32_t i, j, max_match_len;

	/* reset lz77 node */
	node->off = 0;
	node->len = 0;
	node->literal = 0;

	for (i = 0; i < WINDOW_SIZE; i++) {
		/* compute max match length */
		max_match_len = MIN(WINDOW_SIZE - i, len);

		/* compute match */
		for (j = 0; j < max_match_len && window[i + j] == buf[j]; j++);

		/* update best match */
		if (j > node->len) {
			node->off = WINDOW_SIZE - i;
			node->len = j;
			node->literal = buf[j];
		}
	}

	/* no match = literal node */
	if (node->len == 0) {
		node->off = 0;
		node->len = 0;
		node->literal = *buf;
	}
}

/**
 * @brief Compress a buffer with LZ77 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz77_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct byte_stream bs_out = { 0 };
	uint8_t *window, *buf_in;
	uint32_t window_size, i;
	struct lz77_node node;

	/* write uncompressed length first */
	byte_stream_write_u32(&bs_out, htole32(src_len));

	/* set input buffer and initial window */
	buf_in = window = src;

	/* copy first window to destination */
	window_size = src_len < WINDOW_SIZE ? src_len : WINDOW_SIZE;
	for (i = 0; i < window_size; i++)
		byte_stream_write_u8(&bs_out, *buf_in++);

	/* compress nodes */
	while (buf_in < src + src_len) {
		/* find best match */
		__lz77_match(window, buf_in, src + src_len - buf_in - 1, &node);

		/* write match or literal */
		byte_stream_write_u8(&bs_out, node.off);
		byte_stream_write_u8(&bs_out, node.len);
		byte_stream_write_u8(&bs_out, node.literal);

		/* update window and buffer */
		window += node.len + 1;
		buf_in += node.len + 1;
	}

	/* set destination length */
	*dst_len = bs_out.size;

	return bs_out.buf;
}

/**
 * @brief Uncompress a buffer with LZ77 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz77_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	uint8_t *dst, *buf_in, *buf_out;
	struct lz77_node *node;
	uint32_t window_size;

	/* read uncompressed length first */
	*dst_len = le32toh(*((uint32_t *) src));
	src += sizeof(uint32_t);
	src_len -= sizeof(uint32_t);
	
	/* set input buffer */
	buf_in = src;
	
	/* allocate destination buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* copy first window to destination */
	window_size = src_len < WINDOW_SIZE ? src_len : WINDOW_SIZE;
	memcpy(buf_out, buf_in, window_size);
	buf_in += window_size;
	buf_out += window_size;

	/* uncompress nodes */
	while (buf_in < src + src_len) {
		/* read lz77 node */
		node = (struct lz77_node *) buf_in;
		buf_in += sizeof(struct lz77_node);

		/* retrieve match */
		if (node->len > 0) {
			memcpy(buf_out, buf_out - node->off, node->len);
			buf_out += node->len;
		}

		/* set next literal */
		*buf_out++ = node->literal;
	}

	return dst;
}