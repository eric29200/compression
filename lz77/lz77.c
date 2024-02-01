/*
 * LZ77 algorithm = lossless data compression algorithm.
 * This algorithm maintains a sliding window and try to find a matching pattern in the window :
 *   - if a match is found, pattern offset and length are written
 *   - otherwiser, literal is written
 */
#include <string.h>
#include <endian.h>

#include "lz77.h"
#include "../utils/bit_stream.h"
#include "../utils/mem.h"

#define LZ77_MIN_LEN			3
#define LZ77_HASH_SIZE			32768

/**
 * @brief Hash table node.
 */
struct hash_node {
	int 			index;
	struct hash_node *	next;
};

/**
 * @brief Add a node to a hash table.
 * 
 * @param hash_nodes 		nodes
 * @param nr_hash_nodes		number of nodes in hash table
 * @param index 		node index
 * @param head 			head of nodes
 * 
 * @return new node
 */
static struct hash_node *__hash_add_node(struct hash_node *hash_nodes, uint32_t *nr_hash_nodes, uint32_t index, struct hash_node *head)
{
	struct hash_node *node;

	/* get next free node */
	node = &hash_nodes[*nr_hash_nodes];
	*nr_hash_nodes += 1;

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

	return h % LZ77_HASH_SIZE;
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
static struct lz77_node *__lz77_create_match_node(int distance, uint32_t length)
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
 * @param match 		hash match
 * @param buf 			input buffer
 * @param len 			input buffer length
 * @param ptr 			current pointer in input buffer
 * @param max_match_len		maximum match length
 * @param max_match_dist	maximum match distance
 *
 * @return LZ77 node (match or literal)
 */
static struct lz77_node *__lz77_best_match(struct hash_node *hash_match, uint8_t *buf, int len, uint8_t *ptr, uint32_t max_match_len, uint32_t max_match_dist)
{
	uint32_t max, len_max = 0, i;
	struct hash_node *match_max;
	uint8_t *match_buf;

	/* compute maximum match length */
	max = len - (ptr - buf);
	if (max > max_match_len)
		max = max_match_len;

	/* for each match */
	for (; hash_match != NULL; hash_match = hash_match->next) {
		match_buf = buf + hash_match->index;

		/* match too far */
		if (ptr - match_buf > max_match_dist)
			break;

		/* no way to improve best match */
		if (len_max >= max || match_buf[len_max] != ptr[len_max])
			continue;

		/* compute match length */
		for (i = 0; match_buf[i] == ptr[i] && i < max; i++);

		/* update maximum match length */
		if (i > len_max) {
			len_max = i;
			match_max = hash_match;
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
 * @param nr_hash_nodes	number of hash nodes
 * @param hash_table 	hash table
 * @param ptr 		current pointer in input buffer
 * @param len 		number of bytes to skip
 * 
 * @return number of bytes skipped
 */
static int lz77_skip(uint8_t *buf, struct hash_node *hash_nodes, uint32_t *nr_hash_nodes, struct hash_node **hash_table, uint8_t *ptr, uint32_t len)
{
	uint32_t i, index;

	/* hash skipped bytes */
	for (i = 0; i < len; i++) {
		index = __lz77_hash(&ptr[i]);
		hash_table[index] = __hash_add_node(hash_nodes, nr_hash_nodes, ptr + i - buf, hash_table[index]);
	}

	return len;
}

/**
 * @brief Compress a buffer with LZ77 algorithm.
 * 
 * @param buf 			input buffer
 * @param len 			input buffer length
 * @param max_match_len		maximum match length
 * @param max_match_dis		maximum match distance
 * 
 * @return output LZ77 nodes
 */
struct lz77_node *lz77_compress_buf(uint8_t *src, uint32_t src_len, uint32_t max_match_len, uint32_t max_match_dist)
{
	struct lz77_node *lz77_head = NULL, *lz77_tail = NULL;
	struct hash_node **hash_table, *hash_nodes, *hash_match;
	uint32_t i, index, nr_hash_nodes;
	struct lz77_node *lz77_node;
	uint8_t *ptr;

	/* create nodes array */
	hash_nodes = (struct hash_node *) xmalloc(sizeof(struct hash_node) * src_len);
	nr_hash_nodes = 0;

	/* create hash table */
	hash_table = (struct hash_node **) xmalloc(sizeof(struct hash_node *) * LZ77_HASH_SIZE);
	for (i = 0; i < LZ77_HASH_SIZE; i++)
		hash_table[i] = NULL;

	/* find matching patterns */
	for (ptr = src; ptr + LZ77_MIN_LEN - 1 - src < src_len; ptr++) {
		/* compute next 3 characters hash */
		index = __lz77_hash(ptr);

		/* add it to hash table */
		hash_match = hash_table[index];
		hash_table[index] = __hash_add_node(hash_nodes, &nr_hash_nodes, ptr - src, hash_match);

		/* find best match */
		lz77_node = __lz77_best_match(hash_match, src, src_len, ptr, max_match_len, max_match_dist);

		/* add node to list */
		if (!lz77_head) {
			lz77_head = lz77_node;
			lz77_tail = lz77_node;
		} else {
			lz77_tail->next = lz77_node;
			lz77_tail = lz77_node;
		}

		/* skip match */
		if (lz77_node->is_literal == 0)
			ptr += lz77_skip(src, hash_nodes, &nr_hash_nodes, hash_table, ptr, lz77_node->data.match.length - 1);
	}

	/* add remaining bytes */
	for (; ptr - src < src_len; ptr++) {
		/* create a literal node */
		lz77_node = __lz77_create_literal_node(*ptr);

		/* add node to list */
		if (!lz77_head) {
			lz77_head = lz77_node;
			lz77_tail = lz77_node;
		} else {
			lz77_tail->next = lz77_node;
			lz77_tail = lz77_node;
		}
	}

	/* free hash table and nodes */
	xfree(hash_table);
	xfree(hash_nodes);

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
	xfree(node);
}

/**
 * @brief Compress a buffer with LZ77 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz77_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	struct bit_stream bs_out = { 0 };
	struct lz77_node *nodes, *node;

	/* write uncompressed length first */
	bit_stream_write_bits(&bs_out, htole32(src_len), 32, BIT_ORDER_MSB);

	/* compress input buffer */
	nodes = lz77_compress_buf(src, src_len, 255, 32768);

	/* write lz77 nodes */
	for (node = nodes; node != NULL; node = node->next) {
		/* write node type */
		bit_stream_write_bits(&bs_out, node->is_literal, 1, BIT_ORDER_MSB);

		/* write literal */
		if (node->is_literal) {
			bit_stream_write_bits(&bs_out, node->data.literal, 8, BIT_ORDER_MSB);
			continue;
		}
		
		/* write match */
		bit_stream_write_bits(&bs_out, htole16(node->data.match.distance), 16, BIT_ORDER_MSB);
		bit_stream_write_bits(&bs_out, node->data.match.length, 8, BIT_ORDER_MSB);
	}

	/* flush last byte */
	bit_stream_flush(&bs_out);

	/* set destination length */
	*dst_len = bs_out.byte_offset;

	return bs_out.buf;
}

/**
 * @brief Uncompress a buffer with LZ77 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz77_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len)
{
	uint32_t is_literal, distance, length, i, n;
	struct bit_stream bs_in = { 0 };
	uint8_t *buf_out;

	/* set input bit stream */
	bs_in.buf = src;

	/* read uncompressed length first */
	*dst_len = bit_stream_read_bits(&bs_in, 32, BIT_ORDER_MSB);

	/* allocate output buffer */
	buf_out = (uint8_t *) xmalloc(*dst_len);

	/* read lz77 nodes */
	for (n = 0; n < *dst_len && bs_in.byte_offset < src_len;) {
		/* read node type */
		is_literal = bit_stream_read_bits(&bs_in, 1, BIT_ORDER_MSB);

		/* read literal */
		if (is_literal) {
			buf_out[n++] = bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB);
			continue;
		}

		/* read match */
		distance = le16toh(bit_stream_read_bits(&bs_in, 16, BIT_ORDER_MSB));
		length = bit_stream_read_bits(&bs_in, 8, BIT_ORDER_MSB);

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return buf_out;
}