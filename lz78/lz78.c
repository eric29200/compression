/*
 * LZ78 algorithm = lossless data compression algorithm.
 * This algorithm maintains a dictionnary (= a trie).
 * 1 - read each character
 * 2 - if the character is already in the dictionnary (start at root), go to next character and update tree node
 *     if the character is not in the dictionnary, add it to the dictionnary and write the previous node id and then character
 * 3 - write final sequence
 */
#include <string.h>
#include <endian.h>

#include "lz78.h"
#include "../utils/trie.h"
#include "../utils/mem.h"
#include "../utils/byte_stream.h"

/**
 * @brief Decode a node.
 * 
 * @param node 		node
 * @param buf_out 	output buffer
 * 
 * @return number of characters written
 */
static int __decode_node(struct trie *node, uint8_t *buf_out)
{
	struct trie *tmp;
	int i, j;

	/* compute dict entry size */
	for (tmp = node, i = 0; tmp->parent != NULL; tmp = tmp->parent, i++);

	/* write decoded string */
	for (tmp = node, j = 0; tmp->parent != NULL; tmp = tmp->parent, j++)
		buf_out[i - 1 - j] = tmp->val;

	return i;
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
	struct byte_stream bs_out = { 0 };
	uint8_t *buf_in, c;
	int id = 0;

	/* set input buffer */
	buf_in = src;

	/* write uncompressed length first */
	byte_stream_write_u32(&bs_out, htole32(src_len));

	/* write temporary dict size */
	byte_stream_write_u32(&bs_out, htole32(id));

	/* create root node */
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

		/* write node id and next character */
		byte_stream_write_u32(&bs_out, htole32(node->id));
		byte_stream_write_u8(&bs_out, c);

		/* go back to root */
		node = root;
	}

	/* write last node id */
	if (node != root)
		byte_stream_write_u32(&bs_out, node->id);
		
	/* write final dict size */
	*((int *) (bs_out.buf + sizeof(uint32_t))) = htole32(id);

	/* free dictionnary */
	trie_free(root);

	/* set destination length */
	*dst_len = bs_out.size;

	return bs_out.buf;
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
	uint8_t *dst, *buf_in, *buf_out, c;
	int dict_size, id = 0, node_id, i;
	struct trie **dict, *root, *node;

	/* read uncompressed length first */
	*dst_len = le32toh(*((uint32_t *) src));
	src += sizeof(uint32_t);
	src_len -= sizeof(uint32_t);

	/* set input buffer */
	buf_in = src;

	/* allocate output buffer */
	dst = buf_out = (uint8_t *) xmalloc(*dst_len);

	/* get dict size */
	dict_size = le32toh(*((int *) buf_in));
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
		node_id = le32toh(*((int *) buf_in));
		buf_in += sizeof(int);
		
		/* get node */
		node = dict[node_id];

		/* decode node */
		buf_out += __decode_node(node, buf_out);

		/* no next character : exit */
		if (buf_in >= src + src_len)
			break;

		/* read next character */
		c = *buf_in++;

		/* insert new node */
		trie_insert(node, c, id++);
		node = trie_find(node, c);
		dict[node->id] = node;

		/* write next character */
		*buf_out++ = c;
	}

	/* free dictionnary */
	xfree(dict);
	trie_free(root);

	return dst;
}
