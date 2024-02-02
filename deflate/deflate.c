#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include <string.h>

#include "deflate.h"
#include "../lz77/lz77.h"
#include "huffman.h"
#include "no_compression.h"
#include "../utils/bit_stream.h"
#include "../utils/byte_stream.h"
#include "../utils/mem.h"

#define DEFLATE_BLOCK_SIZE			0xFFFF
#define DEFLATE_COMPRESSION_NO			0
#define DEFLATE_COMPRESSION_FIX_HUFFMAN		1
#define DEFLATE_COMPRESSION_DYN_HUFFMAN		2
#define DEFLATE_LZ77_MAX_LEN			258
#define DEFLATE_LZ77_MAX_DIST			32768

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

	return ~crc;
}

/**
 * @brief Compress a block.
 * 
 * @param block 		input block
 * @param block_len 		input block length
 * @param last_block 		last block ?
 * @param bs_fix_huff 		fix huffman bit stream
 * @param bs_dyn_huff 		dynamic huffman bit stream
 * @param bs_no 		no compression bit stream
 * 
 * @return output bit stream (with best compression)
 */
static struct bit_stream *__compress_block(uint8_t *block, uint16_t block_len, int last_block,
					   struct bit_stream *bs_fix_huff,
					   struct bit_stream *bs_dyn_huff,
					   struct bit_stream *bs_no)
{
	struct lz77_node *lz77_nodes;
	struct bit_stream *bs;

	/* lz77 compression */
	lz77_nodes = lz77_compress_buf(block, block_len, DEFLATE_LZ77_MAX_LEN, DEFLATE_LZ77_MAX_DIST);

	/* fix huffman compression */
	bit_stream_write_bits(bs_fix_huff, last_block, 1, BIT_ORDER_LSB);
	bit_stream_write_bits(bs_fix_huff, DEFLATE_COMPRESSION_FIX_HUFFMAN, 2, BIT_ORDER_LSB);
	deflate_huffman_compress(lz77_nodes, bs_fix_huff, 0);

	/* dynamic huffman compression */
	bit_stream_write_bits(bs_dyn_huff, last_block, 1, BIT_ORDER_LSB);
	bit_stream_write_bits(bs_dyn_huff, DEFLATE_COMPRESSION_DYN_HUFFMAN, 2, BIT_ORDER_LSB);
	deflate_huffman_compress(lz77_nodes, bs_dyn_huff, 1);

	/* no compression */
	bit_stream_write_bits(bs_no, last_block, 1, BIT_ORDER_LSB);
	bit_stream_write_bits(bs_no, DEFLATE_COMPRESSION_NO, 2, BIT_ORDER_MSB);
	deflate_no_compression_compress(block, block_len, bs_no);

	/* choose best compression method */
	if (bs_fix_huff->byte_offset <= bs_dyn_huff->byte_offset && bs_fix_huff->byte_offset <= bs_no->byte_offset)
		bs = bs_fix_huff;
	else if (bs_dyn_huff->byte_offset <= bs_no->byte_offset)
		bs = bs_dyn_huff;
	else
		bs = bs_no;

	/* last block : flush last byte */
	if (last_block)
		bit_stream_flush(bs);

	return bs;
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
	struct byte_stream bs_out = { 0 };
	uint16_t block_len;
	int last_block = 0;
	uint8_t *block;

	/* compress block by block */
	for (block = src; block < src + src_len; block += block_len) {
		/* compute block length */
		block_len = DEFLATE_BLOCK_SIZE;
		if (block + block_len >= src + src_len) {
			block_len = src + src_len - block;
			last_block = 1;
		}

		/* compress block */
		bs = __compress_block(block, block_len, last_block, &bs_fix_huff, &bs_dyn_huff, &bs_no);

		/* copy bit stream to output buffer */
		byte_stream_write(&bs_out, bs->buf, bs->byte_offset);

		/* clear bit streams (remember last byte) */
		bs_fix_huff.buf[0] = bs_dyn_huff.buf[0] = bs_no.buf[0] = bs->buf[bs->byte_offset];
		bs_fix_huff.bit_offset = bs_dyn_huff.bit_offset = bs_no.bit_offset = bs->bit_offset;
		bs_fix_huff.byte_offset = bs_dyn_huff.byte_offset = bs_no.byte_offset = 0;
	}

	/* write crc */
	byte_stream_write_u32(&bs_out, htole32(__crc32(src, src_len, ~0)));

	/* write uncompressed length */
	byte_stream_write_u32(&bs_out, htole32(src_len));

	/* free bit streams */
	xfree(bs_fix_huff.buf);
	xfree(bs_dyn_huff.buf);
	xfree(bs_no.buf);

	/* set destination length */
	*dst_len = bs_out.size;

	return bs_out.buf;
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
			case DEFLATE_COMPRESSION_NO:
				buf_out += deflate_no_compression_uncompress(&bs_in, buf_out);
				break;
			case DEFLATE_COMPRESSION_FIX_HUFFMAN:
				buf_out += deflate_huffman_uncompress(&bs_in, buf_out, 0);
				break;
			case DEFLATE_COMPRESSION_DYN_HUFFMAN:
				buf_out += deflate_huffman_uncompress(&bs_in, buf_out, 1);
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
