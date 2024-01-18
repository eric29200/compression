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
	uint8_t *dst;

	/* compress with lz77 algorithm */
	lz77_nodes = deflate_lz77_compress(src, src_len);

	/* compress with fix huffman algorithm */
	deflate_fix_huffman_compress(lz77_nodes, 1, &bs_fix_huff);

	/* compress with dynamic huffman algorithm */
	deflate_fix_huffman_compress(lz77_nodes, 1, &bs_dyn_huff);

	/* choose compression method */
	if (src_len + 4 < bs_fix_huff.byte_offset && src_len + 4 < bs_dyn_huff.byte_offset) {
		deflate_no_compression_compress(src, src_len, 1, &bs_no);
		bs = &bs_no;
	} else if (bs_fix_huff.byte_offset < bs_dyn_huff.byte_offset) {
		bs = &bs_fix_huff;
	} else {
		bs = &bs_dyn_huff;
	}

	/* flush last byte if needed */
	if (bs->bit_offset > 0)
		bs->byte_offset++;

	/* set destination length */
	*dst_len = sizeof(uint32_t) + bs->byte_offset;

	/* allocate output buffer */
	dst = (uint8_t *) xmalloc(*dst_len);

	/* write uncompressed length first */
	*((uint32_t *) dst) = htole32(src_len);

	/* copy bit stream to output buffer */
	memcpy(dst + sizeof(uint32_t), bs->buf, bs->byte_offset);

	/* free bit streams */
	xfree(bs_no.buf);
	xfree(bs_fix_huff.buf);
	xfree(bs_dyn_huff.buf);

	/* free lz77 nodes */
	deflate_lz77_free_nodes(lz77_nodes);

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