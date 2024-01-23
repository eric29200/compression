#ifndef _FIX_HUFFMAN_H_
#define _FIX_HUFFMAN_H_

#include "huffman.h"

/**
 * @brief Build fix huffman tables.
 * 
 * @param table_lit 		literals huffman table
 * @param table_dist 		distances huffman table
 */
void deflate_huffman_build_fix_tables(struct huffman_table *table_lit, struct huffman_table *table_dist);

#endif