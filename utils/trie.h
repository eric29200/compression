#ifndef _TRIE_H_
#define _TRIE_H_

#include <stdint.h>

/**
 * @brief Trie data structure.
 */
struct trie {
	int 			id;		/* node id */
	uint8_t 		val;		/* node value */
	struct trie *		parent;		/* parent node */
	struct trie *		children;	/* children */
	struct trie *		next;		/* next node */
};

/**
 * @brief Insert a node in a trie.
 * 
 * @param root 			root node
 * @param val 			node value
 * @param id 			node id
 * 
 * @return new root
 */
struct trie *trie_insert(struct trie *root, uint8_t val, int id);

/**
 * @brief Free a trie.
 * 
 * @param root 		trie
 */
void trie_free(struct trie *root);

/**
 * @brief Find a node in a trie.
 * 
 * @param root 		root
 * @param val		node value
 * 
 * @return node
 */
struct trie *trie_find(struct trie *root, uint8_t val);

#endif
