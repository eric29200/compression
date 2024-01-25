/*
 * Huffman encoding = lossless data compression method, working at alphabet level :
 * 1 - parse all file to compute frequency of each character
 * 2 - build huffman tree (min heap) based on frequencies
 *	-> every letter is a leaf in the heap
 *	-> more frequent letters have shortest code
 * 3 - build binary code of every letter
 * 4 - write header in compressed file = every letter with its frequency = dictionnary (so decompressor will be able to rebuild the tree)
 * 5 - encode file = replace each letter with binary code
 */

#include <string.h>
#include <endian.h>

#include "huffman.h"
#include "huffman_tree.h"
#include "../utils/mem.h"
#include "../utils/heap.h"
#include "../utils/bit_stream.h"

#define NR_CHARACTERS		256


/**
 * @brief Write huffman header (= dictionnary).
 * 
 * @param src_len	input buffer length
 * @param nodes 	huffman nodes
 * @param header_len	output header length
 */
static uint8_t *__write_huffman_header(uint32_t src_len, struct huffman_node **nodes, uint32_t *header_len)
{
	uint8_t *header, *buf_out;
	uint32_t i, n;

	/* count number of nodes */
	for (i = 0, n = 0; i < NR_CHARACTERS; i++)
		if (nodes[i])
			n++;

	/* allocate header */
	*header_len = sizeof(uint32_t) + sizeof(uint32_t) + n * (sizeof(uint32_t) + sizeof(uint32_t));
	header = buf_out = (uint8_t *) xmalloc(*header_len);

	/* write input buffer length */
	*((uint32_t *) buf_out) = htole32(src_len);
	buf_out += sizeof(uint32_t);

	/* write number of nodes */
	*((uint32_t *) buf_out) = htole32(n);
	buf_out += sizeof(uint32_t);

	/* write dictionnary */
	for (i = 0; i < NR_CHARACTERS; i++) {
		if (!nodes[i])
			continue;

		/* write value */
		*((uint32_t *) buf_out) = htole32(nodes[i]->val);
		buf_out += sizeof(uint32_t);

		/* write frequency */
		*((uint32_t *) buf_out) = htole32(nodes[i]->freq);
		buf_out += sizeof(uint32_t);
	}

	return header;
}

/**
 * @brief Read huffman header (= dictionnary).
 * 
 * @param buf_in	input buffer
 * @param freqs 	output characters frequencies
 * @param dst_len	output destination length
 * 
 * @return length of this header
 */
static uint32_t __read_huffman_header(uint8_t *buf_in, uint32_t *freqs, uint32_t *dst_len)
{
	uint32_t i, n, freq;
	uint32_t val;

	/* read destination length */
	*dst_len = le32toh(*((uint32_t *) buf_in));
	buf_in += sizeof(uint32_t);

	/* read number of nodes */
	n = le32toh(*((uint32_t *) buf_in));
	buf_in += sizeof(uint32_t);

	/* read nodes */
	for (i = 0; i < n; i++) {
		/* read value */
		val = le32toh(*((uint32_t *) buf_in));
		buf_in += sizeof(uint32_t);

		/* read frequency */
		freq = le32toh(*((uint32_t *) buf_in));
		buf_in += sizeof(uint32_t);

		/* set frequency */
		freqs[val] = freq;
	}

	/* return length of this header */
	return sizeof(uint32_t) + sizeof(uint32_t) + n * (sizeof(uint32_t) + sizeof(uint32_t));
}

/**
 * @brief Encode input buffer with huffman codes.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param nodes 	huffman nodes
 * @param bs_out 	output bit stream
 */
static void __write_huffman_content(uint8_t *src, uint32_t src_len, struct huffman_node **nodes, struct bit_stream *bs_out)
{
	struct huffman_node *node;
	uint32_t i;

	for (i = 0; i < src_len; i++) {
		/* get huffman node */
		node = nodes[src[i]];

		/* write huffman code */
		bit_stream_write_bits(bs_out, node->huffman_code, node->nr_bits, BIT_ORDER_MSB);
	}
}

/**
 * @brief Decode a huffman node value.
 * 
 * @param bs_in 	input bit stream
 * @param root	 	huffman tree
 *
 * @return huffman value
 */
static int __read_huffman_val(struct bit_stream *bs_in, struct huffman_node *root)
{
	struct huffman_node *node;
	uint32_t bit;

	for (node = root;;) {
		bit = bit_stream_read_bits(bs_in, 1, BIT_ORDER_MSB);

		/* walk through the tree */
		if (bit)
			node = node->right;
		else
			node = node->left;

		if (__huffman_leaf(node))
			return node->val;
	}

	return -1;
}

/**
 * @brief Decode input buffer with huffman codes.
 * 
 * @param bs_in 	input bit stream
 * @param dst 		output buffer
 * @param dst_len	output buffer length
 * @param root 		huffman tree
 */
static void __read_huffman_content(struct bit_stream *bs_in, uint8_t *dst, uint32_t dst_len, struct huffman_node *root)
{
	uint32_t val, i;

	/* decode each character */
	for (i = 0; i < dst_len; i++) {
		/* get next character */
		val = __read_huffman_val(bs_in, root);

		/* literal */
		dst[i] = val;
	}
}

/**
 * @brief Compress a buffer with huffman algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *huffman_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct huffman_node *tree, *nodes[NR_CHARACTERS] = { NULL };
	uint32_t i, freqs[NR_CHARACTERS] = { 0 };
	struct bit_stream bs_out = { 0 };
	uint8_t *dst;

	/* compute characters frequencies */
	for (i = 0; i < src_len; i++)
		freqs[src[i]]++;

	/* build huffman tree */
	tree = huffman_tree_create(freqs, NR_CHARACTERS);

	/* extract huffman nodes */
	huffman_tree_extract_nodes(tree, nodes);

	/* write huffman header (= write dictionnary with frequencies) */
	dst = __write_huffman_header(src_len, nodes, dst_len);

	/* set output bit stream */
	bs_out.capacity = *dst_len;
	bs_out.buf = dst;
	bs_out.byte_offset = *dst_len;
	bs_out.bit_offset = 0;

	/* write huffman content (= encode input buffer) */
	__write_huffman_content(src, src_len, nodes, &bs_out);

	/* set destination length */
	*dst_len = bs_out.byte_offset + (bs_out.bit_offset ? 1 : 0);

	/* free huffman tree */
	huffman_tree_free(tree);

	return bs_out.buf;
}

/**
 * @brief Uncompress a buffer with huffman algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *huffman_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	uint32_t freqs[NR_CHARACTERS] = { 0 };
	struct bit_stream bs_in = { 0 };
	struct huffman_node *tree;
	uint32_t header_len;
	uint8_t *dst;

	/* read huffman header */
	header_len = __read_huffman_header(src, freqs, dst_len);

	/* allocate output buffer */
	dst = (uint8_t *) xmalloc(*dst_len);

	/* build huffman tree */
	tree = huffman_tree_create(freqs, NR_CHARACTERS);

	/* set input bit stream */
	bs_in.capacity = src_len - header_len;
	bs_in.buf = src + header_len;

	/* decode input buffer */
	__read_huffman_content(&bs_in, dst, *dst_len, tree);

	/* free huffman tree */
	huffman_tree_free(tree);

	return dst;
}
