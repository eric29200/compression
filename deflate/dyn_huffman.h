#ifndef _DYN_HUFFMAN_H_
#define _DYN_HUFFMAN_H_

#include "huffman.h"

/**
 * @brief Build dynamic huffman tables.
 * 
 * @param lz77_nodes		LZ77 nodes
 * @param table_lit 		literals huffman table
 * @param table_dist 		distances huffman table
 */
void deflate_huffman_build_dynamic_tables(struct lz77_node *lz77_nodes, struct huffman_table *table_lit, struct huffman_table *table_dist);

/**
 * @brief Write huffman tables.
 * 
 * @param bs_out 	output bit stream
 * @param table_lit 	huffman literals table
 * @param table_dist	huffman distances table
 */
void deflate_huffman_write_tables(struct bit_stream *bs_out, struct huffman_table *table_lit, struct huffman_table *table_dist);

/**
 * @brief Read huffman tables.
 * 
 * @param bs_in 	input bit stream
 * @param table_lit 	huffman literals table
 * @param table_dist	huffman distances table
 */
void deflate_huffman_read_tables(struct bit_stream *bs_in, struct huffman_table *table_lit, struct huffman_table *table_dist);

#endif
