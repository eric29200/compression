#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_huffman.h"
#include "huffman.h"
#include "../utils/heap.h"
#include "../utils/mem.h"

#define DYN_HUFF_NR_CODES		(256 + 1 + DEFLATE_HUFFMAN_NR_LENGTH_CODES)

#define __huffman_leaf(node)		((node)->left == NULL && (node)->right == NULL)

/**
 * @brief Huffman node.
 */
struct huff_node {
	int 			val;					/* value */
	int 			freq;					/* frequency */
	uint8_t			huff_code[DYN_HUFF_NR_CODES / 8 + 1];	/* huffman code */
	struct huff_node *	left;					/* left node */
	struct huff_node *	right;					/* right node */
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
static struct huff_node *__huff_node_create(int val, int freq)
{
	struct huff_node *node;

	node = xmalloc(sizeof(struct huff_node));
	node->val = val;
	node->freq = freq;
	node->left = NULL;
	node->right = NULL;
	memset(node->huff_code, 0, sizeof(node->huff_code));

	return node;
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
	free(root);

	/* free children */
	__huffman_tree_free(left);
	__huffman_tree_free(right);
}

/**
 * @brief Create a huffman tree.
 * 
 * @param freqs 		characters frequencies
 * @param nr_characters		number of characters
 * 
 * @return huffman tree
 */
static struct huff_node *__huffman_tree(int *freqs, uint32_t nr_characters)
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
		if (!node) {
			heap_free(heap);
			return NULL;
		}

		/* insert node */
		heap_insert(heap, node);
	}

	/* build huffman tree */
	while (heap->size > 1) {
		/* extract 2 minimum values */
		left = heap_min(heap);
		right = heap_min(heap);

		/* build parent node (= left frequency + right frequency)*/
		top = __huff_node_create(-1, left->freq + right->freq);
		if (!top)
			return NULL;

		/* insert parent node in heap */
		top->left = left;
		top->right = right;
		heap_insert(heap, top);
	}

	return heap_min(heap);
}

/**
 * @brief Build huffman codes.
 * 
 * @param root 		huffman tree
 * @param code 		huffman code
 * @param top 		position in huffman code
 */
static void __huffman_tree_build_codes(struct huff_node *root, uint8_t *code, int top)
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
		memcpy(root->huff_code, code, top);
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
 * @brief Compute literales and distances frequencies.
 * 
 * @param lz77_nodes	LZ77 nodes
 * @param freqs_lit 	output literal frequencies
 * @param freqs_dist	output distance frequencies
 */
static void __compute_frequencies(struct lz77_node *lz77_nodes, int *freqs_lit, int *freqs_dist)
{
	struct lz77_node *lz77_node;

	/* compute frequencies */
	for (lz77_node = lz77_nodes; lz77_node != NULL; lz77_node = lz77_node->next) {
		if (lz77_node->is_literal) {
			freqs_lit[lz77_node->data.literal]++;
		} else {
			freqs_lit[257 + deflate_fix_huffman_length_index(lz77_node->data.match.length)]++;
			freqs_dist[deflate_fix_huffman_distance_index(lz77_node->data.match.distance)]++;
		}
	}

	/* add "end of block" character */
	freqs_lit[256]++;
}

/**
 * @brief Write huffman header (= dictionnary).
 * 
 * @param bs_out 	output bit stream
 * @param nodes_lit 	huffman literal nodes
 * @param nodes_dist	huffman distance nodes
 */
static void __write_huffman_header(struct bit_stream *bs_out, struct huff_node **nodes_lit, struct huff_node **nodes_dist)
{
	int i, n;

	/* count number of literal nodes */
	for (i = 0, n = 0; i < DYN_HUFF_NR_CODES; i++)
		if (nodes_lit[i])
			n++;

	/* write number of literal nodes */
	bit_stream_write_bits(bs_out, n, 9);

	/* write literal dictionnary */
	for (i = 0; i < DYN_HUFF_NR_CODES; i++) {
		if (!nodes_lit[i])
			continue;

		bit_stream_write_bits(bs_out, nodes_lit[i]->val, 9);
		bit_stream_write_bits(bs_out, nodes_lit[i]->freq, 16);
	}

	/* count number of distance nodes */
	for (i = 0, n = 0; i < DEFLATE_HUFFMAN_NR_DISTANCE_CODES; i++)
		if (nodes_dist[i])
			n++;

	/* write number of distance nodes */
	bit_stream_write_bits(bs_out, n, 5);

	/* write distance dictionnary */
	for (i = 0, n = 0; i < DEFLATE_HUFFMAN_NR_DISTANCE_CODES; i++) {
		if (!nodes_dist[i])
			continue;

		bit_stream_write_bits(bs_out, nodes_dist[i]->val, 5);
		bit_stream_write_bits(bs_out, nodes_dist[i]->freq, 16);
	}
}

/**
 * @brief Read huffman header (= dictionnary).
 * 
 * @param bs_in 	input bit stream
 * @param freqs_lit 	output literals frequencies
 * @param freqs_dist	output distances frequencies
 */
static void __read_huffman_header(struct bit_stream *bs_in, int *freqs_lit, int *freqs_dist)
{
	int i, n, val;

	/* read number of literal nodes */
	n = bit_stream_read_bits(bs_in, 9);

	/* read literal nodes */
	for (i = 0; i < n; i++) {
		val = bit_stream_read_bits(bs_in, 9);
		freqs_lit[val] = bit_stream_read_bits(bs_in, 16);
	}

	/* read number of distance nodes */
	n = bit_stream_read_bits(bs_in, 5);

	/* read distance nodes */
	for (i = 0; i < n; i++) {
		val = bit_stream_read_bits(bs_in, 5);
		freqs_dist[val] = bit_stream_read_bits(bs_in, 16);
	}
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
		bit_stream_write_bits(bs_out, huff_node->huff_code[i] - '0', 1);
}

/**
 * @brief Encode a block with huffman codes.
 * 
 * @param lz77_nodes	LZ77 nodes
 * @param nodes_lit 	huffman literal nodes
 * @param nodes_dist	huffman distance nodes
 * @param bs_out 	output bit stream
 */
static void __write_huffman_content(struct lz77_node *lz77_nodes, struct huff_node **nodes_lit, struct huff_node **nodes_dist, struct bit_stream *bs_out)
{
	struct lz77_node *lz77_node;

	/* for each lz77 nodes */
	for (lz77_node = lz77_nodes; lz77_node != NULL; lz77_node = lz77_node->next) {
		/* get huffman node and write it to output bit stream */
		if (lz77_node->is_literal) {
			__write_huffman_code(bs_out, nodes_lit[lz77_node->data.literal]);
		} else {
			/* write length */
			__write_huffman_code(bs_out, nodes_lit[257 + deflate_fix_huffman_length_index(lz77_node->data.match.length)]);
			deflate_fix_huffman_encode_length_extra_bits(bs_out, lz77_node->data.match.length);

			/* write distance */
			__write_huffman_code(bs_out, nodes_dist[deflate_fix_huffman_distance_index(lz77_node->data.match.distance)]);
			deflate_fix_huffman_encode_distance_extra_bits(bs_out, lz77_node->data.match.distance);
		}
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
		v = bit_stream_read_bits(bs_in, 1);

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
 * @param buf_out	output buffer
 * @param root_lit	huffman literals tree
 * @param root_dist	huffman distances tree
 */
static int __read_huffman_content(struct bit_stream *bs_in, uint8_t *buf_out, struct huff_node *root_lit, struct huff_node *root_dist)
{
	int literal, length, distance, i, n;

	/* decode each character */
	for (n = 0;;) {
		/* get next literal */
		literal = __read_huffman_val(bs_in, root_lit);

		/* end of block : exit */
		if (literal == 256)
			break;

		/* literal */
		if (literal < 256) {
			buf_out[n++] = literal;
			continue;
		}

		/* decode lz77 length and distance */
		length = deflate_fix_huffman_decode_length(bs_in, literal - 257);
		distance = deflate_fix_huffman_decode_distance(bs_in, __read_huffman_val(bs_in, root_dist));

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return n;
}

/**
 * @brief Compress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param lz77_nodes 		LZ77 nodes
 * @param last_block 		is this last block ?
 * @param bs_out 		output bit stream
 */
void deflate_dyn_huffman_compress(struct lz77_node *lz77_nodes, int last_block, struct bit_stream *bs_out)
{
	int freqs_lit[DYN_HUFF_NR_CODES] = { 0 }, freqs_dist[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = { 0 };
	struct huff_node *huff_nodes_dist[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = { NULL };
	struct huff_node *huff_nodes_lit[DYN_HUFF_NR_CODES] = { NULL };
	struct huff_node *huff_tree_lit, *huff_tree_dist;
	uint8_t code[DYN_HUFF_NR_CODES];

	/* write block header (final block + compression method) */
	bit_stream_write_bits(bs_out, last_block, 1);
	bit_stream_write_bits(bs_out, 2, 2);

	/* compute literals and distances frequencies */
	__compute_frequencies(lz77_nodes, freqs_lit, freqs_dist);

	/* build huffman trees */
	huff_tree_lit = __huffman_tree(freqs_lit, DYN_HUFF_NR_CODES);
	huff_tree_dist = __huffman_tree(freqs_dist, DEFLATE_HUFFMAN_NR_DISTANCE_CODES);

	/* build huffman codes */
	__huffman_tree_build_codes(huff_tree_lit, code, 0);
	__huffman_tree_build_codes(huff_tree_dist, code, 0);

	/* extract huffman nodes */
	__huffman_tree_extract_nodes(huff_tree_lit, huff_nodes_lit);
	__huffman_tree_extract_nodes(huff_tree_dist, huff_nodes_dist);

	/* write huffman header (= write dictionnary with frequencies) */
	__write_huffman_header(bs_out, huff_nodes_lit, huff_nodes_dist);

	/* write huffman content (= encode lz77 data) */
	__write_huffman_content(lz77_nodes, huff_nodes_lit, huff_nodes_dist, bs_out);

	/* write end of block */
	__write_huffman_code(bs_out, huff_nodes_lit[256]);

	/* free huffman trees */
	__huffman_tree_free(huff_tree_lit);
	__huffman_tree_free(huff_tree_dist);
}

/**
 * @brief Uncompress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param bs_in 		input bit stream
 * @param buf_out 		output buffer
 *
 * @return number of bytes written to output buffer
 */
int deflate_dyn_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out)
{
	int freqs_lit[DYN_HUFF_NR_CODES] = { 0 }, freqs_dist[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = { 0 };
	struct huff_node *huff_tree_lit, *huff_tree_dist;
	int len;

	/* read dynamic huffman header (= get frequencies and dictionnary ) */
	__read_huffman_header(bs_in, freqs_lit, freqs_dist);

	/* build huffman trees */
	huff_tree_lit = __huffman_tree(freqs_lit, DYN_HUFF_NR_CODES);
	huff_tree_dist = __huffman_tree(freqs_dist, DEFLATE_HUFFMAN_NR_DISTANCE_CODES);

	/* read/decode huffman content */
	len = __read_huffman_content(bs_in, buf_out, huff_tree_lit, huff_tree_dist);

	/* free huffman trees */
	__huffman_tree_free(huff_tree_lit);
	__huffman_tree_free(huff_tree_dist);

	return len;
}
