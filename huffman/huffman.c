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
#include "../utils/mem.h"
#include "../utils/heap.h"
#include "../utils/bit_stream.h"

#define NR_CHARACTERS		256


/**
 * @brief Compare 2 huffman nodes.
 * 
 * @param h1 		first node
 * @param h2 		second node
 * 
 * @return result
 */
static int __huff_node_compare(const void *h1, const void *h2)
{
	return ((struct huff_node *) h1)->freq - ((struct huff_node *) h2)->freq;
}

/**
 * @brief Create a huffman node.
 * 
 * @param val		value
 * @param freq 		frequency
 *
 * @return huffman node
 */
static struct huff_node *__huff_node_create(uint32_t val, uint32_t freq)
{
	struct huff_node *node;

	node = (struct huff_node *) xmalloc(sizeof(struct huff_node));
	node->val = val;
	node->freq = freq;
	node->left = NULL;
	node->right = NULL;
	node->huff_code = 0;
	node->nr_bits = 0;

	return node;
}

/**
 * @brief Build huffman codes.
 * 
 * @param root 		huffman tree
 * @param code 		huffman code
 * @param nr_bits	number of bits
 */
static void __huffman_tree_build_codes(struct huff_node *root, uint32_t code, uint32_t nr_bits)
{
	/* build huffman code on left (encode with a zero) */
	if (root->left)
		__huffman_tree_build_codes(root->left, code << 1, nr_bits + 1);

	/* build huffman code on right (encode with a one) */
	if (root->right)
		__huffman_tree_build_codes(root->right, (code << 1) | 0x01, nr_bits + 1);

	/* leaf : create code */
	if (__huffman_leaf(root)) {
		root->huff_code = code;
		root->nr_bits = nr_bits;
	}
}

/**
 * @brief Create a huffman tree.
 * 
 * @param freqs 		characters frequencies
 * @param nr_characters		number of characters in the alphabet
 * 
 * @return huffman tree
 */
struct huff_node *huffman_tree_create(uint32_t *freqs, uint32_t nr_characters)
{
	struct huff_node *left, *right, *top, *node;
	struct heap *heap;
	uint32_t i;

	/* create a heap */
	heap = heap_create(HEAP_MIN, nr_characters * 2, __huff_node_compare);
	if (!heap)
		return NULL;

	/* build min heap */
	for (i = 0; i < nr_characters; i++) {
		if (!freqs[i])
			continue;

		/* create node */
		node = __huff_node_create(i, freqs[i]);
		if (!node)
			goto out;

		/* insert node */
		heap_insert(heap, node);
	}

	/* build huffman tree */
	while (heap->size > 1) {
		/* extract 2 minimum values */
		left = heap_min(heap);
		right = heap_min(heap);

		/* build parent node (= left frequency + right frequency)*/
		top = __huff_node_create('$', left->freq + right->freq);
		if (!top)
			return NULL;

		/* insert parent node in heap */
		top->left = left;
		top->right = right;
		heap_insert(heap, top);
	}

	/* get root */
	node = heap_min(heap);

	/* build huffman codes */
	__huffman_tree_build_codes(node, 0, 0);
out:
	heap_free(heap);
	return node;
}

/**
 * @brief Extract huffman nodes from a tree.
 * 
 * @param root		huffman tree
 * @param nodes		output nodes
 */
void huffman_tree_extract_nodes(struct huff_node *root, struct huff_node **nodes)
{
	if (!root)
		return;

	if (root->left)
		huffman_tree_extract_nodes(root->left, nodes);

	if (root->right)
		huffman_tree_extract_nodes(root->right, nodes);

	if (__huffman_leaf(root))
		nodes[root->val] = root;
}


/**
 * @brief Free a huffman tree.
 * 
 * @param root 		root node
 */
void huffman_tree_free(struct huff_node *root)
{
	struct huff_node *left, *right;

	if (!root)
		return;

	/* free root */
	left = root->left;
	right = root->right;
	xfree(root);

	/* free children */
	huffman_tree_free(left);
	huffman_tree_free(right);
}

/**
 * @brief Compute characters frequencies.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param freqs 	output frequencies
 */
static void __compute_frequencies(uint8_t *src, uint32_t src_len, uint32_t *freqs)
{
	uint32_t i;

	for (i = 0; i < src_len; i++)
		freqs[src[i]]++;
}

/**
 * @brief Write huffman header (= dictionnary).
 * 
 * @param src_len	input buffer length
 * @param nodes 	huffman nodes
 * @param header_len	output header length
 */
static uint8_t *__write_huffman_header(uint32_t src_len, struct huff_node **nodes, uint32_t *header_len)
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
static void __write_huffman_content(uint8_t *src, uint32_t src_len, struct huff_node **nodes, struct bit_stream *bs_out)
{
	struct huff_node *node;
	uint32_t i;

	for (i = 0; i < src_len; i++) {
		/* get huffman node */
		node = nodes[src[i]];

		/* write huffman code */
		bit_stream_write_bits(bs_out, node->huff_code, node->nr_bits, BIT_ORDER_MSB);
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
static int __read_huffman_val(struct bit_stream *bs_in, struct huff_node *root)
{
	struct huff_node *node;
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
static void __read_huffman_content(struct bit_stream *bs_in, uint8_t *dst, uint32_t dst_len, struct huff_node *root)
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
	struct huff_node *huff_tree, *nodes[NR_CHARACTERS] = { NULL };
	struct bit_stream bs_out = { 0 };
	uint32_t freqs[NR_CHARACTERS] = { 0 };
	uint8_t *dst;

	/* compute characters frequencies */
	__compute_frequencies(src, src_len, freqs);

	/* build huffman tree */
	huff_tree = huffman_tree_create(freqs, NR_CHARACTERS);

	/* extract huffman nodes */
	huffman_tree_extract_nodes(huff_tree, nodes);

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
	huffman_tree_free(huff_tree);

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
	struct huff_node *huff_tree;
	uint32_t header_len;
	uint8_t *dst;

	/* read huffman header */
	header_len = __read_huffman_header(src, freqs, dst_len);

	/* allocate output buffer */
	dst = (uint8_t *) xmalloc(*dst_len);

	/* build huffman tree */
	huff_tree = huffman_tree_create(freqs, NR_CHARACTERS);

	/* set input bit stream */
	bs_in.capacity = src_len - header_len;
	bs_in.buf = src + header_len;

	/* decode input buffer */
	__read_huffman_content(&bs_in, dst, *dst_len, huff_tree);

	/* free huffman tree */
	huffman_tree_free(huff_tree);

	return dst;
}
