#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <string.h>

#include "deflate.h"
#include "lz77.h"
#include "dyn_huffman.h"
#include "fix_huffman.h"
#include "no_compression.h"
#include "../utils/bit_stream.h"
#include "../utils/mem.h"

#define DEFLATE_BLOCK_SIZE	0xFFFF

/**
 * @brief Compress a buffer with deflate algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *deflate_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct bit_stream bs_fix_huff = { 0 }, bs_dyn_huff = { 0 }, bs_no = { 0 }, *bs;
	uint32_t block_len, dst_capacity, pos;
	struct lz77_node *lz77_nodes = NULL;
	uint8_t *dst, *buf_out, *block;
	int last_block = 0;

	/* allocate output buffer */
	dst_capacity = sizeof(uint32_t);
	dst = buf_out = (uint8_t *) xmalloc(dst_capacity);

	/* write uncompressed length first */
	*((uint32_t *) buf_out) = htole32(src_len);
	buf_out += sizeof(uint32_t);

	/* compress block by block */
	for (block = src; block < src + src_len; block += block_len) {
		/* compute block length */
		block_len = DEFLATE_BLOCK_SIZE;
		if (block + block_len >= src + src_len) {
			block_len = src + src_len - block;
			last_block = 1;
		}

		/* lz77 compression */
		lz77_nodes = deflate_lz77_compress(block, block_len);

		/* fix huffman compression */
		deflate_fix_huffman_compress(lz77_nodes, last_block, &bs_fix_huff);

		/* dynamic huffman compression */
		deflate_dyn_huffman_compress(lz77_nodes, last_block, &bs_dyn_huff);

		/* choose compression method */
		if (block_len + 4 < bs_fix_huff.byte_offset && block_len + 4 < bs_dyn_huff.byte_offset) {
			deflate_no_compression_compress(block, block_len, last_block, &bs_no);
			bs = &bs_no;
		} else if (bs_fix_huff.byte_offset < bs_dyn_huff.byte_offset) {
			bs = &bs_fix_huff;
		} else {
			bs = &bs_dyn_huff;
		}

		/* flush last byte */
		bit_stream_flush(bs);

		/* grow buffer if needed */
		if (buf_out - dst + bs->byte_offset > dst_capacity) {
			/* remember position */
			pos = buf_out - dst;

			/* grow destination */
			dst_capacity = buf_out - dst + bs->byte_offset;
			dst = xrealloc(dst, dst_capacity);

			/* set new position */
			buf_out = dst + pos;
		}

		/* copy bit stream to output buffer */
		memcpy(buf_out, bs->buf, bs->byte_offset);
		buf_out += bs->byte_offset;

		/* reset fix huffman bit stream */
		bs_fix_huff.byte_offset = 0;
		bs_fix_huff.bit_offset = 0;

		/* reset dynamic huffman bit stream */
		bs_dyn_huff.byte_offset = 0;
		bs_dyn_huff.bit_offset = 0;

		/* reset no compression bit stream */
		bs_no.byte_offset = 0;
		bs_no.bit_offset = 0;

		/* free lz77 nodes */
		deflate_lz77_free_nodes(lz77_nodes);
	}

	/* free bit streams */
	xfree(bs_fix_huff.buf);
	xfree(bs_dyn_huff.buf);
	xfree(bs_no.buf);

	/* set destination length */
	*dst_len = buf_out - dst;

	return dst;
}

/**
 * @brief Uncompress a buffer with deflate algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *deflate_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct bit_stream bs_in = { 0 };
	uint8_t *dst, *buf_out;
	int last_block, type;

	/* read uncompressed length first */
	*dst_len = le32toh(*((uint32_t *) src));
	src += sizeof(uint32_t);
	src_len -= sizeof(uint32_t);

	/* allocate output buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* set input bit stream */
	bs_in.buf = src;
	bs_in.capacity = src_len;

	/* uncompress block by block */
	for (;;) {
		/* get block header */
		last_block = bit_stream_read_bits(&bs_in, 1);
		type = bit_stream_read_bits(&bs_in, 2);

		/* handle compression type */
		switch (type) {
			case 0:
				buf_out += deflate_no_compression_uncompress(&bs_in, buf_out);
				break;
			case 1:
				buf_out += deflate_fix_huffman_uncompress(&bs_in, buf_out);
				break;
			case 2:
				buf_out += deflate_dyn_huffman_uncompress(&bs_in, buf_out);
				break;
			default:
				goto err;
		}
	
		/* last block : exit */
		if (last_block)
			break;

		/* go to next byte */
		if (bs_in.bit_offset) {
			bs_in.byte_offset++;
			bs_in.bit_offset = 0;
		}
	}

	return dst;
err:
	*dst_len = 0;
	xfree(dst);
	return NULL;
}
