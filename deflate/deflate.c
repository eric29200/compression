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
	struct lz77_node *lz77_nodes;
	uint32_t dst_capacity;
	uint8_t *dst, *block;
	uint16_t block_len;
	int last_block = 0;

	/* allocate output buffer */
	dst_capacity = sizeof(uint32_t);
	dst = (uint8_t *) xmalloc(dst_capacity);
	*dst_len = 0;

	/* write uncompressed length first */
	*((uint32_t *) dst + *dst_len) = htole32(src_len);
	*dst_len += sizeof(uint32_t);

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

		/* no compression */
		deflate_no_compression_compress(block, block_len, last_block, &bs_no);

		/* choose best compression method */
		bs = bs_fix_huff.byte_offset <= bs_dyn_huff.byte_offset ? &bs_fix_huff : &bs_dyn_huff;
		bs = bs_no.byte_offset <= bs->byte_offset ? &bs_no : bs;

		/* last block : flush last byte */
		if (last_block)
			bit_stream_flush(bs);

		/* grow output buffer if needed */
		if (*dst_len + bs->byte_offset > dst_capacity) {
			dst_capacity = *dst_len + bs->byte_offset;
			dst = xrealloc(dst, dst_capacity);
		}

		/* copy bit stream to output buffer */
		memcpy(dst + *dst_len, bs->buf, bs->byte_offset);
		*dst_len += bs->byte_offset;

		/* clear bit streams (remember last byte) */
		bs_fix_huff.buf[0] = bs_dyn_huff.buf[0] = bs_no.buf[0] = bs->buf[bs->byte_offset];
		bs_fix_huff.bit_offset = bs_dyn_huff.bit_offset = bs_no.bit_offset = bs->bit_offset;
		bs_fix_huff.byte_offset = bs_dyn_huff.byte_offset = bs_no.byte_offset = 0;

		/* free lz77 nodes */
		deflate_lz77_free_nodes(lz77_nodes);
	}

	/* free bit streams */
	xfree(bs_fix_huff.buf);
	xfree(bs_dyn_huff.buf);
	xfree(bs_no.buf);

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
	}

	return dst;
err:
	*dst_len = 0;
	xfree(dst);
	return NULL;
}
