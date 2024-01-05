#ifndef _LZ77_H_
#define _LZ77_H_

#include <stdio.h>
#include <stdint.h>

#define LZ77_MAX_DISTANCE	32768
#define LZ77_MIN_LEN		3
#define LZ77_MAX_LEN		256
/*
 * LZ77 match.
 */
struct lz77_match {
	int 				distance;
	int 				length;
};

/*
 * LZ77 node (literal or match).
 */
struct lz77_node {
	int 				is_literal;
	union {
		uint8_t 		literal;
		struct lz77_match 	match;
	} data;
	struct lz77_node *		next;
};

struct lz77_node *deflate_lz77_compress(uint8_t *buf, int len);
void deflate_lz77_free_nodes(struct lz77_node *node);

#endif
