/*
 * LZW algorithm = lossless data compression algorithm.
 * Same as LZ78 but starts with single characters dictionnary = no need to emit next character.
 */
#include <string.h>
#include <endian.h>

#include "lzw.h"
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
	struct trie *root = NULL, *node, *next;
	struct byte_stream bs_out = { 0 };
	uint8_t *buf_in, c;
	int id = 0, i;

	/* set input buffer */
	buf_in = src;

	/* write uncompressed length first */
	byte_stream_write_u32(&bs_out, htole32(src_len));

	/* write temporary dict size */
	byte_stream_write_u32(&bs_out, htole32(id));

	/* create root node */
	root = node = trie_insert(NULL, 0, id++);

	/* insert single characters at root */
	for (i = 0; i < 256; i++, id++)
		trie_insert(root, (uint8_t) i, id);

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

		/* write node id */
		byte_stream_write_u32(&bs_out, htole32(node->id));

		/* go back to current character */
		node = trie_find(root, c);
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
	struct trie **dict, *root, *node, *prev, *tmp;
	int dict_size, id = 0, node_id, node_len, i;
	uint8_t *dst, *buf_in, *buf_out;

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
	root = prev = dict[0] = trie_insert(NULL, 0, id++);

	/* insert single characters at root */
	for (i = 0; i < 256; i++, id++) {
		trie_insert(root, (uint8_t) i, id);
		tmp = trie_find(root, (uint8_t) i);
		dict[tmp->id] = tmp;
	}

	/* uncompress */
	while (buf_in < src + src_len) {
		/* read node id */
		node_id = le32toh(*((int *) buf_in));
		buf_in += sizeof(int);

		/* get node */
		node = dict[node_id];

		/* decode node */
		node_len = __decode_node(node, buf_out);

		if (prev != root) {
			trie_insert(prev, buf_out[0], id++);
			tmp = trie_find(prev, buf_out[0]);
			dict[tmp->id] = tmp;
		}

		buf_out += node_len;

		/* remember previous node */
		prev = node;
	}

	/* free dictionnary */
	xfree(dict);
	trie_free(root);

	return dst;
}
