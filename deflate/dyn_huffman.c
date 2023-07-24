#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_huffman.h"
#include "huffman.h"
#include "../utils/heap.h"
#include "../utils/mem.h"

#define DYN_HUFF_NB_CODES		(256 + 1 + HUFF_NB_LENGTH_CODES)

#define huffman_leaf(node)		((node)->left == NULL && (node)->right == NULL)

/*
 * Huffman node.
 */
struct huff_node_t {
	int 			item;
	int 			freq;
	char 			huff_code[DYN_HUFF_NB_CODES / 8 + 1];
	struct huff_node_t *	left;
	struct huff_node_t *	right;
};

/*
 * Compare 2 huffman nodes.
 */
static int huff_node_compare(const void *h1, const void *h2)
{
	return ((struct huff_node_t *) h1)->freq - ((struct huff_node_t *) h2)->freq;
}

/*
 * Create a new huffman node.
 */
static struct huff_node_t *huff_node_create(int item, int freq)
{
	struct huff_node_t *node;

	node = xmalloc(sizeof(struct huff_node_t));
	node->item = item;
	node->freq = freq;
	node->left = NULL;
	node->right = NULL;
	memset(node->huff_code, 0, sizeof(node->huff_code));

	return node;
}

/*
 * Free a huffman tree.
 */
static void huffman_tree_free(struct huff_node_t *root)
{
	struct huff_node_t *left, *right;

	if (!root)
		return;

	/* free root */
	left = root->left;
	right = root->right;
	free(root);

	/* free children */
	huffman_tree_free(left);
	huffman_tree_free(right);
}

/*
 * Build huffman tree.
 */
static struct huff_node_t *huffman_tree(int *freq, size_t nb_characters)
{
	struct huff_node_t *left, *right, *top, *node;
	struct heap_t *heap;
	size_t i;

	/* create a heap */
	heap = heap_create(HEAP_MIN, nb_characters * 2, huff_node_compare);
	if (!heap)
		return NULL;

	/* build min heap */
	for (i = 0; i < nb_characters; i++) {
		if (freq[i]) {
			/* create node */
			node = huff_node_create(i, freq[i]);
			if (!node) {
				heap_free(heap);
				return NULL;
			}

			/* insert node */
			heap_insert(heap, node);
		}
	}

	/* build huffman tree */
	while (heap->size > 1) {
		/* extract 2 minimum values */
		left = heap_min(heap);
		right = heap_min(heap);

		/* build parent node (= left frequency + right frequency)*/
		top = huff_node_create(-1, left->freq + right->freq);
		if (!top)
			return NULL;

		/* insert parent node in heap */
		top->left = left;
		top->right = right;
		heap_insert(heap, top);
	}

	return heap_min(heap);
}

/*
 * Build huffman codes.
 */
static void huffman_tree_build_codes(struct huff_node_t *root, char *code, int top)
{
	/* build huffman code on left (encode with a zero) */
	if (root->left) {
		code[top] = '0';
		huffman_tree_build_codes(root->left, code, top + 1);
	}

	/* build huffman code on right (encode with a one) */
	if (root->right) {
		code[top] = '1';
		huffman_tree_build_codes(root->right, code, top + 1);
	}

	/* leaf : create code */
	if (huffman_leaf(root))
		memcpy(root->huff_code, code, top);
}

/*
 * Extract huffman nodes from a tree.
 */
static void huffman_tree_extract_nodes(struct huff_node_t *root, struct huff_node_t **nodes)
{
	if (!root)
		return;

	if (root->left)
		huffman_tree_extract_nodes(root->left, nodes);

	if (root->right)
		huffman_tree_extract_nodes(root->right, nodes);

	if (huffman_leaf(root))
		nodes[(int) root->item] = root;
}

/*
 * Compute literals and distances frequencies.
 */
static void __compute_frequencies(struct lz77_node_t *lz77_nodes, int *freq_lit, int *freq_dist)
{
	struct lz77_node_t *lz77_node;

	/* compute frequencies */
	for (lz77_node = lz77_nodes; lz77_node != NULL; lz77_node = lz77_node->next) {
		if (lz77_node->is_literal) {
			freq_lit[lz77_node->data.literal]++;
		} else {
			freq_lit[257 + huff_length_index(lz77_node->data.match.length)]++;
			freq_dist[huff_distance_index(lz77_node->data.match.distance)]++;
		}
	}

	/* add "end of block" character */
	freq_lit[256]++;
}

/*
 * Write dynamic huffman header (items and frequencies).
 */
static void __write_huff_header(struct huff_node_t **huff_nodes_lit, struct huff_node_t **huff_nodes_dist, struct bit_stream_t *bs_out)
{
	int i, n;

	/* count number of literal nodes */
	for (i = 0, n = 0; i < DYN_HUFF_NB_CODES; i++)
		if (huff_nodes_lit[i] != NULL)
			n++;

	/* write number of literal nodes */
	bit_stream_write_bits(bs_out, n, 9);

	/* write literal dictionnary */
	for (i = 0; i < DYN_HUFF_NB_CODES; i++) {
		if (huff_nodes_lit[i] != NULL) {
			bit_stream_write_bits(bs_out, huff_nodes_lit[i]->item, 9);
			bit_stream_write_bits(bs_out, huff_nodes_lit[i]->freq, 16);
		}
	}

	/* count number of distance nodes */
	for (i = 0, n = 0; i < HUFF_NB_DISTANCE_CODES; i++)
		if (huff_nodes_dist[i] != NULL)
			n++;

	/* write number of distance nodes */
	bit_stream_write_bits(bs_out, n, 5);

	/* write distance dictionnary */
	for (i = 0, n = 0; i < HUFF_NB_DISTANCE_CODES; i++) {
		if (huff_nodes_dist[i] != NULL) {
			bit_stream_write_bits(bs_out, huff_nodes_dist[i]->item, 5);
			bit_stream_write_bits(bs_out, huff_nodes_dist[i]->freq, 16);
		}
	}
}

/*
 * Read dynamic huffman header (items and frequencies).
 */
static void __read_huff_header(struct bit_stream_t *bs_in, int *freq_lit, int *freq_dist)
{
	int i, n, item;

	/* read number of literal nodes */
	n = bit_stream_read_bits(bs_in, 9);

	/* read literal nodes */
	for (i = 0; i < n; i++) {
		item = bit_stream_read_bits(bs_in, 9);
		freq_lit[item] = bit_stream_read_bits(bs_in, 16);
	}

	/* read number of distance nodes */
	n = bit_stream_read_bits(bs_in, 5);

	/* read distance nodes */
	for (i = 0; i < n; i++) {
		item = bit_stream_read_bits(bs_in, 5);
		freq_dist[item] = bit_stream_read_bits(bs_in, 16);
	}
}

/*
 * Write a huffman code.
 */
static void __write_huff_code(struct huff_node_t *huff_node, struct bit_stream_t *bs_out)
{
	int i;

	for (i = 0; huff_node->huff_code[i]; i++)
		bit_stream_write_bit(bs_out, huff_node->huff_code[i] - '0');
}

/*
 * Encode a block with huffman codes.
 */
static void __write_huff_content(struct lz77_node_t *lz77_nodes, struct huff_node_t **huff_nodes_lit, struct huff_node_t **huff_nodes_dist, struct bit_stream_t *bs_out)
{
	struct lz77_node_t *lz77_node;

	/* for each lz77 nodes */
	for (lz77_node = lz77_nodes; lz77_node != NULL; lz77_node = lz77_node->next) {
		/* get huffman node and write it to output bit stream */
		if (lz77_node->is_literal) {
			__write_huff_code(huff_nodes_lit[lz77_node->data.literal], bs_out);
		} else {
			/* write length */
			__write_huff_code(huff_nodes_lit[257 + huff_length_index(lz77_node->data.match.length)], bs_out);
			huff_encode_length_extra_bits(lz77_node->data.match.length, bs_out);

			/* write distance */
			__write_huff_code(huff_nodes_dist[huff_distance_index(lz77_node->data.match.distance)], bs_out);
			huff_encode_distance_extra_bits(lz77_node->data.match.distance, bs_out);
		}
	}
}

/*
 * Decode next huffman item.
 */
static int __read_huff_item(struct huff_node_t *huff_tree, struct bit_stream_t *bs_in)
{
	struct huff_node_t *node;
	int v;

	for (node = huff_tree;;) {
		v = bit_stream_read_bit(bs_in);

		/* walk through the tree */
		if (v)
			node = node->right;
		else
			node = node->left;

		if (huffman_leaf(node))
			return node->item;
	}

	return -1;
}


/*
 * Decode a block with huffman codes.
 */
static int __read_huff_content(struct huff_node_t *huff_tree_lit, struct huff_node_t *huff_tree_dist, struct bit_stream_t *bs_in, char *buf_out)
{
	int literal, length, distance, i, n;

	/* decode each character */
	for (n = 0;;) {
		/* get next literal */
		literal = __read_huff_item(huff_tree_lit, bs_in);

		/* end of block : exit */
		if (literal == 256)
			break;

		/* literal */
		if (literal < 256) {
			buf_out[n++] = literal;
			continue;
		}

		/* decode lz77 length and distance */
		length = huff_decode_length(literal - 257, bs_in);
		distance = huff_decode_distance(__read_huff_item(huff_tree_dist, bs_in), bs_in);

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return n;
}

/*
 * Compress lz77 nodes with dynamic huffman alphabet.
 */
void dyn_huffman_compress(struct lz77_node_t *lz77_nodes, int last_block, struct bit_stream_t *bs_out)
{
	int freq_lit[DYN_HUFF_NB_CODES] = { 0 }, freq_dist[HUFF_NB_DISTANCE_CODES] = { 0 };
	struct huff_node_t *huff_nodes_dist[HUFF_NB_DISTANCE_CODES] = { NULL };
	struct huff_node_t *huff_nodes_lit[DYN_HUFF_NB_CODES] = { NULL };
	struct huff_node_t *huff_tree_lit, *huff_tree_dist;
	char code[DYN_HUFF_NB_CODES];

	/* write block header */
	if (last_block)
		bit_stream_write_bits(bs_out, 6, 3);
	else
		bit_stream_write_bits(bs_out, 2, 3);

	/* compute literals and distances frequencies */
	__compute_frequencies(lz77_nodes, freq_lit, freq_dist);

	/* build huffman trees */
	huff_tree_lit = huffman_tree(freq_lit, DYN_HUFF_NB_CODES);
	huff_tree_dist = huffman_tree(freq_dist, HUFF_NB_DISTANCE_CODES);

	/* build huffman codes */
	huffman_tree_build_codes(huff_tree_lit, code, 0);
	huffman_tree_build_codes(huff_tree_dist, code, 0);

	/* extract huffman nodes */
	huffman_tree_extract_nodes(huff_tree_lit, huff_nodes_lit);
	huffman_tree_extract_nodes(huff_tree_dist, huff_nodes_dist);

	/* write huffman header (= write dictionnary with frequencies) */
	__write_huff_header(huff_nodes_lit, huff_nodes_dist, bs_out);

	/* write huffman content (= encode lz77 data) */
	__write_huff_content(lz77_nodes, huff_nodes_lit, huff_nodes_dist, bs_out);

	/* write end of block */
	__write_huff_code(huff_nodes_lit[256], bs_out);

	/* free huffman trees */
	huffman_tree_free(huff_tree_lit);
	huffman_tree_free(huff_tree_dist);
}

/*
 * Uncompress lz77 nodes with fix huffman codes.
 */
int dyn_huffman_uncompress(struct bit_stream_t *bs_in, char *buf_out)
{
	int freq_lit[DYN_HUFF_NB_CODES] = { 0 }, freq_dist[HUFF_NB_DISTANCE_CODES] = { 0 };
	struct huff_node_t *huff_tree_lit, *huff_tree_dist;
	int len;

	/* read dynamic huffman header (= get frequencies and dictionnary ) */
	__read_huff_header(bs_in, freq_lit, freq_dist);

	/* build huffman trees */
	huff_tree_lit = huffman_tree(freq_lit, DYN_HUFF_NB_CODES);
	huff_tree_dist = huffman_tree(freq_dist, HUFF_NB_DISTANCE_CODES);

	/* read/decode huffman content */
	len = __read_huff_content(huff_tree_lit, huff_tree_dist, bs_in, buf_out);

	/* free huffman trees */
	huffman_tree_free(huff_tree_lit);
	huffman_tree_free(huff_tree_dist);

	return len;
}
