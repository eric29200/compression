#include <stdio.h>
#include <stdlib.h>

#include "trie.h"
#include "../utils/mem.h"

/**
 * @brief Allocate a new trie.
 * 
 * @param val 			node value
 * @param id 			node id
 * @param parent 		parent node
 * 
 * @return new trie
 */
static struct trie *__trie_alloc(uint8_t val, int id, struct trie *parent)
{
	struct trie *trie;

	/* allocate a new trie */
	trie = (struct trie *) xmalloc(sizeof(struct trie));
	trie->val = val;
	trie->id = id;
	trie->parent = parent;
	trie->children = NULL;
	trie->next = NULL;

	return trie;
}

/**
 * @brief Insert a node in a trie.
 * 
 * @param root 			root node
 * @param val 			node value
 * @param id 			node id
 * 
 * @return new root
 */
struct trie *trie_insert(struct trie *root, uint8_t val, int id)
{
	struct trie *node;

	/* allocate root if needed */
	if (!root)
		return __trie_alloc(val, id, NULL);

	/* check if node already exists */
	for (node = root->children; node != NULL; node = node->next) {
		if (node->val == val)
			return root;

		if (!node->next)
			break;
	}

	/* add new node */
	if (node)
		node->next = __trie_alloc(val, id, root);
	else
		root->children = __trie_alloc(val, id, root);


	return root;
}

/**
 * @brief Free a trie.
 * 
 * @param root 		trie
 */
void trie_free(struct trie *root)
{
	struct trie *child, *next;

	if (!root)
		return;

	/* free children */
	for (child = root->children; child != NULL; child = next) {
		next = child->next;
		trie_free(child);
	}

	/* free node */
	free(root);
}

/**
 * @brief Find a node in a trie.
 * 
 * @param root 		root
 * @param val		node value
 * 
 * @return node
 */
struct trie *trie_find(struct trie *root, uint8_t val)
{
	struct trie *node;

	if (!root || !root->children)
		return NULL;

	for (node = root->children; node != NULL; node = node->next)
		if (node->val == val)
			return node;

	return NULL;
}
