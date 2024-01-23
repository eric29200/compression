#ifndef _HUFFMAN_TREE_H_
#define _HUFFMAN_TREE_H_

#include <stdio.h>
#include <stdint.h>

#define __huffman_leaf(node)	((node)->left == NULL && (node)->right == NULL)

/**
 * @brief Huffman node.
 */
struct huff_node {
	uint32_t		val;				/* value */
	uint32_t 		freq;				/* frequency */
	uint32_t		huff_code;			/* hufman code */
	uint32_t		nr_bits;			/* number of bits in huffman code */
	struct huff_node *	left;				/* left node */
	struct huff_node *	right;				/* right node */
};

/**
 * @brief Create a huffman tree.
 * 
 * @param freqs 		characters frequencies
 * @param nr_characters		number of characters in the alphabet
 * 
 * @return huffman tree
 */
struct huff_node *huffman_tree_create(uint32_t *freqs, uint32_t nr_characters);

/**
 * @brief Free a huffman tree.
 * 
 * @param root 		root node
 */
void huffman_tree_free(struct huff_node *root);

/**
 * @brief Extract huffman nodes from a tree.
 * 
 * @param root		huffman tree
 * @param nodes		output nodes
 */
void huffman_tree_extract_nodes(struct huff_node *root, struct huff_node **nodes);

#endif