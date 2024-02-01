#ifndef _LZ77_H_
#define _LZ77_H_

#include <stdio.h>
#include <stdint.h>

/**
 * @brief LZ77 match.
 */
struct lz77_match {
	uint32_t 			distance;
	uint32_t 			length;
};

/**
 * @brief LZ77 node (literal or match).
 */
struct lz77_node {
	int 				is_literal;
	union {
		uint8_t 		literal;
		struct lz77_match 	match;
	} data;
	struct lz77_node *		next;
};

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
struct lz77_node *lz77_compress_buf(uint8_t *src, uint32_t src_len, uint32_t max_match_len, uint32_t max_match_dist);

/**
 * @brief Free LZ77 nodes.
 * 
 * @param node 		LZ77 nodes
 */
void deflate_lz77_free_nodes(struct lz77_node *node);

/**
 * @brief Compress a buffer with LZ77 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz77_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len);

/**
 * @brief Uncompress a buffer with LZ77 algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *lz77_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len);

#endif