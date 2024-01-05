#include <stdio.h>
#include <stdlib.h>

#include "lz77.h"
#include "../utils/mem.h"

#define HASH_SIZE	32768

/*
 * Hash table node.
 */
struct hash_node {
	int 			index;
	struct hash_node *	next;
};

/*
 * Add a node to a hash table.
 */
static struct hash_node *__hash_add_node(struct hash_node *nodes, int *nodes_count, int index, struct hash_node *head)
{
	struct hash_node *node;

	/* get next free node */
	node = &nodes[*nodes_count];
	*nodes_count += 1;

	/* set node */
	node->index = index;
	node->next = head;

	return node;
}

/**
 * @brief Hash 3 characters.
 * 
 * @param s 	characters to hash
 * 
 * @return hash code
 */
static inline int __lz77_hash(uint8_t *s)
{
	int h, i;

	for (i = 1, h = *s++; i < LZ77_MIN_LEN; i++)
		h = (h << 5) - h + *s++;

	return h % HASH_SIZE;
}

/**
 * @brief Create a LZ77 literal node.
 * 
 * @param c 	literal
 * 
 * @return LZ77 node
 */
static struct lz77_node *__lz77_create_literal_node(uint8_t c)
{
	struct lz77_node *node;

	node = (struct lz77_node *) xmalloc(sizeof(struct lz77_node));
	node->is_literal = 1;
	node->data.literal = c;
	node->next = NULL;

	return node;
}

/**
 * @brief Create a LZ77 match node.
 * 
 * @param distance	distance from current position
 * @param length	match length
 * 
 * @return LZ77 node
 */
static struct lz77_node *__lz77_create_match_node(int distance, int length)
{
	struct lz77_node *node;

	node = (struct lz77_node *) xmalloc(sizeof(struct lz77_node));
	node->is_literal = 0;
	node->data.match.distance = distance;
	node->data.match.length = length;
	node->next = NULL;

	return node;
}

/**
 * @brief Find best match.
 * 
 * @param match 	hash match
 * @param buf 		input buffer
 * @param len 		input buffer length
 * @param ptr 		current pointer in input buffer
 *
 * @return LZ77 node (match or literal)
 */
static struct lz77_node *__lz77_best_match(struct hash_node *match, uint8_t *buf, int len, uint8_t *ptr)
{
	struct hash_node *match_max;
	int i, len_max = 0, max;
	uint8_t *match_buf;

	/* compute maximum match length */
	max = len - (ptr - buf);
	if (max > LZ77_MAX_LEN)
		max = LZ77_MAX_LEN;

	/* for each match */
	for (; match != NULL; match = match->next) {
		match_buf = buf + match->index;

		/* match too far */
		if (ptr - match_buf > LZ77_MAX_DISTANCE)
			break;

		/* no way to improve best match */
		if (len_max >= max || match_buf[len_max] != ptr[len_max])
			continue;

		/* compute match length */
		for (i = 0; match_buf[i] == ptr[i] && i < max; i++);

		/* update maximum match length */
		if (i > len_max) {
			len_max = i;
			match_max = match;
		}
	}

	/* match too short : create a literal */
	if (len_max < LZ77_MIN_LEN)
		return __lz77_create_literal_node(*ptr);

	/* create a match node */
	return __lz77_create_match_node(ptr - (buf + match_max->index), len_max);
}

/**
 * @brief Skip 'len' bytes (and hash skipped bytes).
 * 
 * @param buf 		input buffer
 * @param nodes 	hash nodes
 * @param nodes_count 	number of hash nodes
 * @param hash_table 	hash table
 * @param ptr 		current pointer in input buffer
 * @param len 		number of bytes to skip
 * 
 * @return number of bytes skipped
 */
static int lz77_skip(uint8_t *buf, struct hash_node *nodes, int *nodes_count, struct hash_node **hash_table, uint8_t *ptr, int len)
{
	int i, index;

	/* hash skipped bytes */
	for (i = 0; i < len; i++) {
		index = __lz77_hash(&ptr[i]);
		hash_table[index] = __hash_add_node(nodes, nodes_count, ptr + i - buf, hash_table[index]);
	}

	return len;
}

/**
 * @brief Compress a buffer with LZ77 algorithm.
 * 
 * @param buf 		input buffer
 * @param len 		input buffer length
 * 
 * @return output LZ77 nodes
 */
struct lz77_node *deflate_lz77_compress(uint8_t *buf, int len)
{
	struct lz77_node *lz77_head = NULL, *lz77_tail = NULL;
	struct hash_node **hash_table, *nodes, *match;
	int i, index, nodes_count;
	struct lz77_node *node;
	uint8_t *ptr;

	/* create nodes array */
	nodes = (struct hash_node *) xmalloc(sizeof(struct hash_node) * len);
	nodes_count = 0;

	/* create hash table */
	hash_table = (struct hash_node **) xmalloc(sizeof(struct hash_node *) * HASH_SIZE);
	for (i = 0; i < HASH_SIZE; i++)
		hash_table[i] = NULL;

	/* find matching patterns */
	for (ptr = buf; ptr + LZ77_MIN_LEN - 1 - buf < len; ptr++) {
		/* compute next 3 characters hash */
		index = __lz77_hash(ptr);

		/* add it to hash table */
		match = hash_table[index];
		hash_table[index] = __hash_add_node(nodes, &nodes_count, ptr - buf, match);

		/* find best match */
		node = __lz77_best_match(match, buf, len, ptr);

		/* add node to list */
		if (!lz77_head) {
			lz77_head = node;
			lz77_tail = node;
		} else {
			lz77_tail->next = node;
			lz77_tail = node;
		}

		/* skip match */
		if (node->is_literal == 0)
			ptr += lz77_skip(buf, nodes, &nodes_count, hash_table, ptr, node->data.match.length - 1);
	}

	/* add remaining bytes */
	for (; ptr - buf < len; ptr++) {
		/* create a literal node */
		node = __lz77_create_literal_node(*ptr);

		/* add node to list */
		if (!lz77_head) {
			lz77_head = node;
			lz77_tail = node;
		} else {
			lz77_tail->next = node;
			lz77_tail = node;
		}
	}

	/* free hash table and nodes */
	free(hash_table);
	free(nodes);

	/* return lz77 nodes */
	return lz77_head;
}

/**
 * @brief Free LZ77 nodes.
 * 
 * @param node 		LZ77 nodes
 */
void deflate_lz77_free_nodes(struct lz77_node *node)
{
	if (!node)
		return;

	deflate_lz77_free_nodes(node->next);
	free(node);
}
