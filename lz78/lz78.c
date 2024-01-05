/*
 * LZ78 algorithm = lossless data compression algorithm.
 * This algorithm maintains a dictionnary (= a trie).
 * 1 - read each character
 * 2 - if the character is already in the dictionnary (start at root), go to next character and update tree node
 *     if the character is not in the dictionnary, add it to the dictionnary and write the previous node id and then character
 * 3 - write final sequence
 */
#include <string.h>

#include "lz78.h"
#include "trie.h"
#include "../utils/mem.h"

#define GROW_SIZE		64
#define MAX(x, y)		((x) > (y) ? (x) : (y))

/**
 * @brief Compress a buffer with LZ78 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz78_compress(uint8_t *src, size_t src_len, size_t *dst_len)
{
	struct trie *root = NULL, *node, *next;
	uint8_t *dst, *buf_in, *buf_out, c;
	size_t dst_capacity, pos;
	int id = 0;

	/* set input buffer */
	buf_in = src;

	/* allocate destination buffer */
	dst_capacity = MAX(src_len, 32);
	dst = buf_out = (uint8_t *) xmalloc(dst_capacity);

	/* write uncompressed length first */
	*((size_t *) buf_out) = src_len;
	buf_out += sizeof(size_t);

	/* write temporary dict size */
	*((int *) buf_out) = id;
	buf_out += sizeof(int);

	/* insert root node */
	root = node = trie_insert(root, 0, id++);

	/* create dictionnary */
	while (buf_in < src + src_len) {
		/* get next character */
		c = *buf_in++;

		/* find character in trie */
		next = trie_find(node, c);
		if (next && buf_in < src + src_len) {
			node = next;
			continue;
		}

		/* insert new character in trie */
		trie_insert(node, c, id++);

		/* check if we need to grow destination buffer */
		if ((size_t) (buf_out - dst + sizeof(int) + 1) > dst_capacity) {
			/* remember position */
			pos = buf_out - dst;

			dst_capacity += GROW_SIZE;
			dst = xrealloc(dst, dst_capacity);
			
			/* set new position */
			buf_out = dst + pos;
		}

		/* write compressed data */
		*((int *) buf_out) = node->id;
		buf_out += sizeof(int);
		*buf_out++ = c;

		/* go back to root */
		node = root;
	}

	/* write final dict size */
	*((int *) (dst + sizeof(size_t))) = id;

	/* free dictionnary */
	trie_free(root);

	/* set destination length */
	*dst_len = buf_out - dst;

	return dst;
}

/**
 * @brief Uncompress a buffer with LZ78 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz78_uncompress(uint8_t *src, size_t src_len, size_t *dst_len)
{
	struct trie **dict, *root, *parent, *node;
	int dict_size, id = 0, parent_id, i, j;
	uint8_t *dst, *buf_in, *buf_out, c;

	/* read uncompressed length first */
	*dst_len = *((size_t *) src);
	src += sizeof(size_t);
	src_len -= sizeof(size_t);

	/* set input buffer */
	buf_in = src;

	/* allocate output buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* get dict size */
	dict_size = *((int *) buf_in);
	buf_in += sizeof(int);

	/* create dict */
	dict = (struct trie **) xmalloc(sizeof(struct trie *) * dict_size);
	for (i = 0; i < dict_size; i++)
		dict[i] = NULL;

	/* insert root node */
	root = dict[0] = trie_insert(NULL, 0, id++);

	/* uncompress */
	while (buf_in < src + src_len) {
		/* read lz78 pair */
		parent_id = *((int *) buf_in);
		buf_in += sizeof(int);
		c = *buf_in++;

		/* get parent */
		parent = dict[parent_id];

		/* insert new node */
		trie_insert(parent, c, id++);
		node = trie_find(parent, c);
		dict[node->id] = node;

		/* compute dict entry size */
		for (node = parent, i = 0; node->parent != NULL; node = node->parent, i++);

		/* write decoded string */
		for (node = parent, j = 0; node->parent != NULL; node = node->parent, j++)
			buf_out[i - 1 - j] = node->val;

		buf_out += i;

		/* write next character */
		*buf_out++ = c;
	}

	/* free dictionnary */
	xfree(dict);
	trie_free(root);

	return dst;
}
