#ifndef _DYN_HUFFMAN_H_
#define _DYN_HUFFMAN_H_

#include <stdint.h>

#include "lz77.h"
#include "../utils/bit_stream.h"

/**
 * @brief Compress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param lz77_nodes 		LZ77 nodes
 * @param last_block 		is this last block ?
 * @param bs_out 		output bit stream
 */
void deflate_dyn_huffman_compress(struct lz77_node *lz77_nodes, int last_block, struct bit_stream *bs_out);

/**
 * @brief Uncompress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param bs_in 		input bit stream
 * @param buf_out 		output buffer
 *
 * @return number of bytes written to output buffer
 */
int deflate_dyn_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out);

#endif
