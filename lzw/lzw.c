/*
 * LZW algorithm = lossless data compression algorithm.
 * This algorithm maintains a dictionnary (= a trie), as LZ78 algorithm.
 * The dictionnary is init with all single characters (256 entries) : the compressed file stores only node's ids and no characters.
 */
#include <string.h>

#include "lzw.h"
#include "../utils/trie.h"
#include "../utils/mem.h"

#define NR_CHARACTERS		256
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
 * @brief Compress a buffer with LZW algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lzw_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct trie *root_tries[NR_CHARACTERS], *cur_trie, *next;
	uint8_t *dst, *buf_in, *buf_out, c;
	uint32_t dst_capacity;
	int id = 0, i;

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

	/* create root tries (one for each single character) */
	for (i = 0; i < NR_CHARACTERS; i++)
		root_tries[i] = trie_insert(NULL, (uint8_t) i, id++);

	/* start with first character */
	cur_trie = root_tries[*buf_in++];

	/* create dictionnary */
	while (buf_in < src + src_len) {
		/* get next character */
		c = *buf_in++;

		/* if character is found in the trie, go down in trie and go to next character */
		next = trie_find(cur_trie, c);
		if (next) {
			cur_trie = next;
			continue;
		}

		/* insert new character in trie */
		trie_insert(cur_trie, c, id++);

		/* grow destination buffer if needed */
		__grow_buffer(&dst, &buf_out, &dst_capacity, sizeof(int));

		/* write trie id */
		*((int *) buf_out) = cur_trie->id;
		buf_out += sizeof(int);

		/* remember next character */
		cur_trie = root_tries[c];
	}

	/* write last characters */
	__grow_buffer(&dst, &buf_out, &dst_capacity, sizeof(int));
	*((int *) buf_out) = cur_trie->id;
	buf_out += sizeof(int);

	/* write final dict size */
	*((int *) (dst + sizeof(uint32_t))) = id;

	/* free dictionnary */
	for (i = 0; i < NR_CHARACTERS; i++)
		trie_free(root_tries[i]);

	/* set destination length */
	*dst_len = buf_out - dst;

	return dst;
}

/**
 * @brief Uncompress a buffer with LZW algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lzw_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct trie **dict, *node, *prev_node = NULL, *tmp, *root;
	int dict_size, id = 0, node_id, i, j;
	uint8_t *dst, *buf_in, *buf_out;

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

	/* create dictionnary */
	dict = (struct trie **) xmalloc(sizeof(struct trie *) * dict_size);
	for (i = 0; i < dict_size; i++)
		dict[i] = i < NR_CHARACTERS ? trie_insert(NULL, (uint8_t) i, id++) : NULL;

	/* uncompress */
	while (buf_in < src + src_len) {
		/* read node id */
		node_id = *((int *) buf_in);
		buf_in += sizeof(int);

		/* get node */
		node = dict[node_id];

		/* compute dict entry size and get root node = next character in input stream */
		for (tmp = node, i = 0, root = NULL; tmp != NULL; tmp = tmp->parent, i++)
			if (tmp->parent == NULL)
				root = tmp;

		/* write decoded string */
		for (tmp = node, j = 0; tmp != NULL; tmp = tmp->parent, j++)
			buf_out[i - 1 - j] = tmp->val;

		/* update output buffer position */
		buf_out += i;

		/* insert node in trie */
		if (prev_node) {
			trie_insert(prev_node, root->val, id);
			tmp = trie_find(prev_node, root->val);
			dict[id++] = tmp;
		}

		/* remember previous node */
		prev_node = node;
	}

	/* free dictionnary */
	for (i = 0; i < NR_CHARACTERS; i++)
		trie_free(dict[i]);
	xfree(dict);

	return dst;
}
