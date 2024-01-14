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
#include "../utils/trie.h"
#include "../utils/mem.h"

#define GROW_SIZE		64
#define MAX(x, y)		((x) > (y) ? (x) : (y))
  
/**
 * @brief Grow output buffer if needed.
 * 
 * @param dst 			output buffer
 * @param buf_out 		current position in output buffer
 * @param dst_capacity 		output buffer capacity
 * @param size_needed		size needed
 */
static void __grow_buffer(uint8_t **dst, uint8_t **buf_out, uint32_t *dst_capacity, uint32_t size_needed)
{
	uint32_t pos;

	/* no need to grower buffer */
	if ((uint32_t) (*buf_out - *dst + size_needed) <= *dst_capacity)
		return;

	/* remember position */
	pos = *buf_out - *dst;

	/* reallocate destination buffer */
	*dst_capacity += GROW_SIZE;
	*dst = xrealloc(*dst, *dst_capacity);
	
	/* set new position */
	*buf_out = *dst + pos;
}

/**
 * @brief Compress a buffer with LZ78 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz78_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct trie *root = NULL, *node, *next;
	uint8_t *dst, *buf_in, *buf_out, c;
	uint32_t dst_capacity;
	int id = 0;

	/* set input buffer */
	buf_in = src;

	/* allocate destination buffer */
	dst_capacity = MAX(src_len, 32);
	dst = buf_out = (uint8_t *) xmalloc(dst_capacity);

	/* write uncompressed length first */
	*((uint32_t *) buf_out) = src_len;
	buf_out += sizeof(uint32_t);

	/* write temporary dict size */
	*((int *) buf_out) = id;
	buf_out += sizeof(int);

	/* insert root node */
	root = node = trie_insert(NULL, 0, id++);

	/* create dictionnary */
	while (buf_in < src + src_len) {
		/* get next character */
		c = *buf_in++;

		/* if character is found in the trie, remember node and go to next character */
		next = trie_find(node, c);
		if (next) {
			node = next;
			continue;
		}

		/* insert new character in trie */
		trie_insert(node, c, id++);

		/* grow destination buffer if needed */
		__grow_buffer(&dst, &buf_out, &dst_capacity, sizeof(int) + sizeof(uint8_t));

		/* write node id and next character */
		*((int *) buf_out) = node->id;
		buf_out += sizeof(int);
		*buf_out++ = c;

		/* go back to root */
		node = root;
	}

	/* write last node id */
	if (node != root) {
		/* grow destination buffer if needed */
		__grow_buffer(&dst, &buf_out, &dst_capacity, sizeof(int));

		/* write node id */
		*((int *) buf_out) = node->id;
		buf_out += sizeof(int);
	}

	/* write final dict size */
	*((int *) (dst + sizeof(uint32_t))) = id;

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
uint8_t *lz78_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct trie **dict, *root, *parent, *node;
	int dict_size, id = 0, parent_id, i, j;
	uint8_t *dst, *buf_in, *buf_out, c;

	/* read uncompressed length first */
	*dst_len = *((uint32_t *) src);
	src += sizeof(uint32_t);
	src_len -= sizeof(uint32_t);

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
		/* read node id */
		parent_id = *((int *) buf_in);
		buf_in += sizeof(int);
		
		/* get parent node */
		parent = dict[parent_id];

		/* compute dict entry size */
		for (node = parent, i = 0; node->parent != NULL; node = node->parent, i++);

		/* write decoded string */
		for (node = parent, j = 0; node->parent != NULL; node = node->parent, j++)
			buf_out[i - 1 - j] = node->val;
		
		/* update output buffer position */
		buf_out += i;

		/* no next character : exit */
		if (buf_in >= src + src_len)
			break;

		/* read next character */
		c = *buf_in++;

		/* insert new node */
		trie_insert(parent, c, id++);
		node = trie_find(parent, c);
		dict[node->id] = node;

		/* write next character */
		*buf_out++ = c;
	}

	/* free dictionnary */
	xfree(dict);
	trie_free(root);

	return dst;
}
