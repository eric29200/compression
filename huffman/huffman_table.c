#include <string.h>

#include "huffman_table.h"
#include "../utils/mem.h"

/**
 * @brief Create a huffman table.
 * 
 * @param table 	huffman table
 * @param len 		length
 */
void huffman_table_create(struct huffman_table *table, uint32_t len)
{
	/* allocate huffman table */
	table->len = len;
	table->codes = (uint32_t *) xmalloc(len * sizeof(uint32_t));
	table->codes_len = (uint32_t *) xmalloc(len * sizeof(uint32_t));

	/* clear huffman table */
	memset(table->codes, 0, len * sizeof(uint32_t));
	memset(table->codes_len, 0, len * sizeof(uint32_t));
}

/**
 * @brief Build a huffman table from a huffman tree.
 * 
 * @param tree			input huffman tree
 * @param table			output huffman table
 * @param len			huffman table length
 */
void huffman_table_build(struct huffman_node *tree, struct huffman_table *table, uint32_t len)
{
	struct huffman_node **nodes;
	uint32_t val, i;

	/* extract huffman nodes */
	nodes = (struct huffman_node **) xmalloc(sizeof(struct huffman_node *) * len);
	memset(nodes, 0, sizeof(struct huffman_node *) * len);
	huffman_tree_extract_nodes(tree, nodes);

	/* create huffman table */
	huffman_table_create(table, len);

	/* build huffman table */
	for (i = 0; i < len; i++) {
		if (nodes[i]) {
			val = nodes[i]->val;
			table->codes[val] = nodes[i]->huffman_code;
			table->codes_len[val] = nodes[i]->nr_bits;
		}
	}

	/* free nodes */
	xfree(nodes);
}

/**
 * @brief Free a huffman table.
 * 
 * @param table 	huffman table
 */
void huffman_table_free(struct huffman_table *table)
{
	if (table) {
		xfree(table->codes);
		xfree(table->codes_len);
	}
}

/**
 * @brief Read and decode a symbol.
 * 
 * @param bs_in		input bit stream
 * @param table		huffman table
 * 
 * @return symbol
 */
int huffman_table_read_symbol(struct bit_stream *bs_in, struct huffman_table *table)
{
	uint32_t code = 0, code_len = 0, i;

	for (;;) {
		/* read next bit */
		code |= bit_stream_read_bits(bs_in, 1, BIT_ORDER_MSB);
		code_len++;

		/* try to find code in huffman table */
		for (i = 0; i < table->len; i++)
			if (table->codes_len[i] == code_len && table->codes[i] == code)
				return i;

		/* go to next bit */
		code <<= 1;
	}
}
