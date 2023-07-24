#ifndef _LZ77_H_
#define _LZ77_H_

#include <stdio.h>

#define LZ77_MAX_DISTANCE	32768
#define LZ77_MIN_LEN		3
#define LZ77_MAX_LEN		256
/*
 * LZ77 match.
 */
struct lz77_match_t {
	int 				distance;
	int 				length;
};

/*
 * LZ77 node (literal or match).
 */
struct lz77_node_t {
	int 				is_literal;
	union {
		unsigned char 		literal;
		struct lz77_match_t 	match;
	} data;
	struct lz77_node_t *		next;
};

struct lz77_node_t *lz77_compress(char *buf, int len);
void lz77_free_nodes(struct lz77_node_t *node);

#endif
