#ifndef _LZ77_H_
#define _LZ77_H_

#include <stdio.h>
#include <stdint.h>

#define LZ77_MAX_DISTANCE	32768
#define LZ77_MIN_LEN		3
#define LZ77_MAX_LEN		258

/**
 * @brief LZ77 match.
 */
struct lz77_match {
	int 				distance;
	int 				length;
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
 * @param buf 		input buffer
 * @param len 		input buffer length
 * 
 * @return output LZ77 nodes
 */
struct lz77_node *deflate_lz77_compress(uint8_t *buf, int len);

/**
 * @brief Free LZ77 nodes.
 * 
 * @param node 		LZ77 nodes
 */
void deflate_lz77_free_nodes(struct lz77_node *node);

#endif
