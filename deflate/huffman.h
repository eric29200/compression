#ifndef _DEFLATE_HUFFMAN_H_
#define _DEFLATE_HUFFMAN_H_

#include "lz77.h"
#include "../utils/bit_stream.h"

#define NR_LITERALS		286
#define NR_LENGTHS 		29
#define NR_DISTANCES		30

/**
 * @brief Huffman table.
 */
struct huffman_table {
	int			len;				/* table length */
	int 			codes[NR_LITERALS];		/* values to huffman codes */
	int 			codes_len[NR_LITERALS];		/* values to huffman codes lengths (= number of bits) */
};

/**
 * @brief Get huffman distance index.
 * 
 * @param distance	distance
 */
int deflate_huffman_distance_index(int distance);

/**
 * @brief Get huffman length index.
 * 
 * @param length	length
 * 
 * @return index
 */
int deflate_huffman_length_index(int length);

/**
 * @brief Compress LZ77 nodes with huffman alphabet.
 * 
 * @param lz77_nodes 		LZ77 nodes
 * @param bs_out 		output bit stream
 * @param dynamic		use dynamic alphabet ?
 */
void deflate_huffman_compress(struct lz77_node *lz77_nodes, struct bit_stream *bs_out, int dynamic);

/**
 * @brief Uncompress LZ77 nodes with huffman alphabet.
 * 
 * @param bs_in 		input bit stream
 * @param buf_out 		output buffer
 * @param dynamic		use dynamic alphabet ?
 *
 * @return number of bytes written to output buffer
 */
int deflate_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out, int dynamic);

#endif
