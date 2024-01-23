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

#include "huffman_tree.h"
#include "../utils/mem.h"
#include "../utils/heap.h"
#include "../utils/bit_stream.h"

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