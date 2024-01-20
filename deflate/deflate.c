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
 * @brief CRC table.
 */
static const uint32_t crc32_tab[16] = {
	0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC, 0x76DC4190,
	0x6B6B51F4, 0x4DB26158, 0x5005713C, 0xEDB88320, 0xF00F9344,
	0xD6D6A3E8, 0xCB61B38C, 0x9B64C2B0, 0x86D3D2D4, 0xA00AE278,
	0xBDBDF21C
};

/**
 * @brief Compute CRC of a buffer.
 * 
 * @param buf 		input buffer
 * @param length 	input buffer length
 * @param crc 		initial value
 * 
 * @return crc
 */
uint32_t __crc32(const uint8_t *buf, uint32_t length, uint32_t crc)
{
	uint32_t i;

	for (i = 0; i < length; i++) {
		crc ^= buf[i];
		crc = crc32_tab[crc & 0x0F] ^ (crc >> 4);
		crc = crc32_tab[crc & 0x0F] ^ (crc >> 4);
	}

	return crc;
}

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
	dst_capacity = *dst_len = 0;
	dst = NULL;

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

	/* grow output buffer if needed (for crc and source length) */
	if (*dst_len + sizeof(uint32_t) * 2 > dst_capacity) {
		dst_capacity = *dst_len + sizeof(uint32_t) * 2;
		dst = xrealloc(dst, dst_capacity);
	}

	/* write crc */
	*((uint32_t *) (dst + *dst_len)) = htole32(__crc32(src, src_len, ~0));
	*dst_len += sizeof(uint32_t);

	/* write uncompressed length */
	*((uint32_t *) (dst + *dst_len)) = htole32(src_len);
	*dst_len += sizeof(uint32_t);

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
	uint32_t crc;

	/* read uncompressed length first */
	*dst_len = le32toh(*((uint32_t *) (src + src_len - sizeof(uint32_t))));
	src_len -= sizeof(uint32_t);

	/* then read crc */
	crc = le32toh(*((uint32_t *) (src + src_len - sizeof(uint32_t))));
	src_len -= sizeof(uint32_t);

	/* allocate output buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* set input bit stream */
	bs_in.buf = src;
	bs_in.capacity = src_len;

	/* uncompress block by block */
	for (;;) {
		/* get block header */
		last_block = bit_stream_read_bits(&bs_in, 1, BIT_ORDER_LSB);
		type = bit_stream_read_bits(&bs_in, 2, BIT_ORDER_LSB);

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

	/* check crc */
	if (__crc32(dst, *dst_len, ~0) != crc)
		goto err;

	return dst;
err:
	*dst_len = 0;
	xfree(dst);
	return NULL;
}
