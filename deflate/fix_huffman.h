#ifndef _FIX_HUFFMAN_H_
#define _FIX_HUFFMAN_H_

#include <stdint.h>

#include "lz77.h"
#include "../utils/bit_stream.h"

/**
 * @brief Compress lz77 nodes with fix huffman alphabet.
 * 
 * @param lz77_nodes 	LZ77 nodes
 * @param last_block 	is this last block ?
 * @param bs_out 	output bit stream
 */
void deflate_fix_huffman_compress(struct lz77_node *lz77_nodes, int last_block, struct bit_stream *bs_out);

/**
 * @brief Uncompress LZ77 nodes with fix huffman codes.
 * 
 * @param bs_in 	input bit stream
 * @param buf_out 	output buffer
 * 
 * @return number of bytes written in output buffer
 */
int deflate_fix_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out);

#endif
