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

#include "huffman.h"
#include "../utils/mem.h"
#include "../utils/heap.h"
#include "../utils/bit_stream.h"

#define NR_CHARACTERS		256

#define __huffman_leaf(node)	((node)->left == NULL && (node)->right == NULL)


/**
 * @brief Huffman node.
 */
struct huff_node {
	uint8_t			val;				/* value */
	int 			freq;				/* frequency */
	char			huff_code[NR_CHARACTERS / 8];	/* huffman code */
	struct huff_node *	left;				/* left node */
	struct huff_node *	right;				/* right node */
};

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
static struct huff_node *__huff_node_create(uint8_t val, int freq)
{
	struct huff_node *node;

	node = (struct huff_node *) xmalloc(sizeof(struct huff_node));
	node->val = val;
	node->freq = freq;
	node->left = NULL;
	node->right = NULL;
	memset(node->huff_code, 0, sizeof(node->huff_code));

	return node;
}

/**
 * @brief Create a huffman tree.
 * 
 * @param freqs 	characters frequencies
 * 
 * @return huffman tree
 */
static struct huff_node *__huffman_tree(int *freqs)
{
	struct huff_node *left, *right, *top, *node;
	struct heap *heap;
	size_t i;

	/* create a heap */
	heap = heap_create(HEAP_MIN, NR_CHARACTERS * 2, __huff_node_compare);
	if (!heap)
		return NULL;

	/* build min heap */
	for (i = 0; i < NR_CHARACTERS; i++) {
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

	/* return min node */
	node = heap_min(heap);
out:
	heap_free(heap);
	return node;
}

/**
 * @brief Build huffman codes.
 * 
 * @param root 		huffman tree
 * @param code 		huffman code
 * @param top 		position in huffman code
 */
static void __huffman_tree_build_codes(struct huff_node *root, char *code, int top)
{
	/* build huffman code on left (encode with a zero) */
	if (root->left) {
		code[top] = '0';
		__huffman_tree_build_codes(root->left, code, top + 1);
	}

	/* build huffman code on right (encode with a one) */
	if (root->right) {
		code[top] = '1';
		__huffman_tree_build_codes(root->right, code, top + 1);
	}

	/* leaf : create code */
	if (__huffman_leaf(root))
		strncpy(root->huff_code, code, top);
}

/**
 * @brief Extract huffman nodes from a tree.
 * 
 * @param root		huffman tree
 * @param nodes		output nodes
 */
static void __huffman_tree_extract_nodes(struct huff_node *root, struct huff_node **nodes)
{
	if (!root)
		return;

	if (root->left)
		__huffman_tree_extract_nodes(root->left, nodes);

	if (root->right)
		__huffman_tree_extract_nodes(root->right, nodes);

	if (__huffman_leaf(root))
		nodes[root->val] = root;
}


/**
 * @brief Free a huffman tree.
 * 
 * @param root 		root node
 */
static void __huffman_tree_free(struct huff_node *root)
{
	struct huff_node *left, *right;

	if (!root)
		return;

	/* free root */
	left = root->left;
	right = root->right;
	xfree(root);

	/* free children */
	__huffman_tree_free(left);
	__huffman_tree_free(right);
}

/**
 * @brief Compute characters frequencies.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param freqs 	output frequencies
 */
static void __compute_frequencies(uint8_t *src, size_t src_len, int *freqs)
{
	size_t i;

	for (i = 0; i < src_len; i++)
		freqs[src[i]]++;
}

/**
 * @brief Write huffman header (= dictionnary).
 * 
 * @param bs_out 	output bit stream
 * @param src_len	input buffer length
 * @param nodes 	huffman nodes
 */
static void __write_huffman_header(struct bit_stream *bs_out, size_t src_len, struct huff_node **nodes)
{
	int i, n;

	/* write input buffer length */
	bit_stream_write_bits(bs_out, src_len, sizeof(size_t) * 8, 1);

	/* count number of nodes */
	for (i = 0, n = 0; i < NR_CHARACTERS; i++)
		if (nodes[i])
			n++;

	/* write number of nodes */
	bit_stream_write_bits(bs_out, n, sizeof(int) * 8, 1);

	/* write dictionnary */
	for (i = 0; i < NR_CHARACTERS; i++) {
		if (!nodes[i])
			continue;

		bit_stream_write_bits(bs_out, nodes[i]->val, sizeof(uint8_t) * 8, 1);
		bit_stream_write_bits(bs_out, nodes[i]->freq, sizeof(int) * 8, 1);
	}
}

/**
 * @brief Read huffman header (= dictionnary).
 * 
 * @param bs_in 	input bit stream
 * @param freqs 	output characters frequencies
 * 
 * @return destination length
 */
static size_t __read_huffman_header(struct bit_stream *bs_in, int *freqs)
{
	size_t dst_len;
	uint8_t val;
	int i, n;

	/* read destination length */
	dst_len = bit_stream_read_bits(bs_in, sizeof(size_t) * 8);

	/* read number of nodes */
	n = bit_stream_read_bits(bs_in, sizeof(int) * 8);

	/* read nodes */
	for (i = 0; i < n; i++) {
		val = bit_stream_read_bits(bs_in, sizeof(uint8_t) * 8);
		freqs[val] = bit_stream_read_bits(bs_in, sizeof(int) * 8);
	}

	return dst_len;
}

/**
 * @brief Write a huffman code.
 * 
 * @param bs_out 	output bit stream
 * @param huff_node 	huffman node
 */
static void __write_huffman_code(struct bit_stream *bs_out, struct huff_node *huff_node)
{
	int i;

	for (i = 0; huff_node->huff_code[i]; i++)
		bit_stream_write_bit(bs_out, huff_node->huff_code[i] - '0', 1);
}

/**
 * @brief Encode input buffer with huffman codes.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param nodes 	huffman nodes
 * @param bs_out 	output bit stream
 */
static void __write_huffman_content(uint8_t *src, size_t src_len, struct huff_node **nodes, struct bit_stream *bs_out)
{
	struct huff_node *node;
	size_t i;

	for (i = 0; i < src_len; i++) {
		/* get huffman node */
		node = nodes[src[i]];

		/* write huffman code */
		__write_huffman_code(bs_out, node);
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
	int v;

	for (node = root;;) {
		v = bit_stream_read_bit(bs_in);

		/* walk through the tree */
		if (v)
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
static void __read_huffman_content(struct bit_stream *bs_in, uint8_t *dst, size_t dst_len, struct huff_node *root)
{
	size_t i;
	int val;

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
uint8_t *huffman_compress(uint8_t *src, size_t src_len, size_t *dst_len)
{
	struct huff_node *huff_tree, *nodes[NR_CHARACTERS] = { NULL };
	int freqs[NR_CHARACTERS] = { 0 };
	char code[NR_CHARACTERS];
	struct bit_stream bs_out;

	/* create output buffer */
	bs_out.buf = NULL;
	bs_out.capacity = 0;
	bs_out.byte_offset = 0;
	bs_out.bit_offset = 0;

	/* compute characters frequencies */
	__compute_frequencies(src, src_len, freqs);

	/* build huffman tree */
	huff_tree = __huffman_tree(freqs);

	/* build huffman codes */
	__huffman_tree_build_codes(huff_tree, code, 0);

	/* extract huffman nodes */
	__huffman_tree_extract_nodes(huff_tree, nodes);

	/* write huffman header (= write dictionnary with frequencies) */
	__write_huffman_header(&bs_out, src_len, nodes);

	/* write huffman content (= encode input buffer) */
	__write_huffman_content(src, src_len, nodes, &bs_out);

	/* set destination length */
	*dst_len = bs_out.byte_offset + (bs_out.bit_offset ? 1 : 0);

	/* free huffman tree */
	__huffman_tree_free(huff_tree);

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
uint8_t *huffman_uncompress(uint8_t *src, size_t src_len, size_t *dst_len)
{
	int freqs[NR_CHARACTERS] = { 0 };
	struct huff_node *huff_tree;
	struct bit_stream bs_in;
	uint8_t *dst;

	/* set input buffer */
	bs_in.buf = src;
	bs_in.capacity = src_len;
	bs_in.byte_offset = 0;
	bs_in.bit_offset = 0;

	/* read huffman header */
	*dst_len = __read_huffman_header(&bs_in, freqs);

	/* allocate output buffer */
	dst = (uint8_t *) xmalloc(*dst_len);

	/* build huffman tree */
	huff_tree = __huffman_tree(freqs);

	/* decode input buffer */
	__read_huffman_content(&bs_in, dst, *dst_len, huff_tree);

	/* free huffman tree */
	__huffman_tree_free(huff_tree);

	return dst;
}