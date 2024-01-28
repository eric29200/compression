#ifndef _HUFFMAN_TABLE_H_
#define _HUFFMAN_TABLE_H_

#include <stdint.h>

#include "huffman_tree.h"
#include "../utils/bit_stream.h"

/**
 * @brief Huffman table.
 */
struct huffman_table {
	uint32_t		len;		/* table length */
	uint32_t * 		codes;		/* values to huffman codes */
	uint32_t * 		codes_len;	/* values to huffman codes lengths (= number of bits) */
};

/**
 * @brief Create a huffman table.
 * 
 * @param table 	huffman table
 * @param len 		length
 */
void huffman_table_create(struct huffman_table *table, uint32_t len);

/**
 * @brief Build a huffman table from a huffman tree.
 * 
 * @param tree			input huffman tree
 * @param table			output huffman table
 * @param len			huffman table length
 */
void huffman_table_build_from_tree(struct huffman_node *tree, struct huffman_table *table, uint32_t len);

/**
 * @brief Build a huffman table from codes lengths.
 * 
 * @param codes_len 		codes lengths
 * @param nr_codes 		number of codes
 * @param table 		output huffman table
 */
void huffman_table_build_from_lengths(uint32_t *codes_len, uint32_t nr_codes, struct huffman_table *table);

/**
 * @brief Free a huffman table.
 * 
 * @param table 	huffman table
 */
void huffman_table_free(struct huffman_table *table);

/**
 * @brief Read and decode a symbol.
 * 
 * @param bs_in		input bit stream
 * @param table		huffman table
 * 
 * @return symbol
 */
int huffman_table_read_symbol(struct bit_stream *bs_in, struct huffman_table *table);

#endif