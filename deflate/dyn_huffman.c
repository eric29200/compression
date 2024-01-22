#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_huffman.h"
#include "huffman.h"
#include "../huffman/huffman.h"
#include "../utils/heap.h"
#include "../utils/mem.h"

#define DYN_HUFF_NR_CODES		(256 + 1 + DEFLATE_HUFFMAN_NR_LENGTH_CODES)

/**
 * @brief Compute literales and distances frequencies.
 * 
 * @param lz77_nodes	LZ77 nodes
 * @param freqs_lit 	output literal frequencies
 * @param freqs_dist	output distance frequencies
 */
static void __compute_frequencies(struct lz77_node *lz77_nodes, uint32_t *freqs_lit, uint32_t *freqs_dist)
{
	struct lz77_node *lz77_node;

	/* compute frequencies */
	for (lz77_node = lz77_nodes; lz77_node != NULL; lz77_node = lz77_node->next) {
		if (lz77_node->is_literal) {
			freqs_lit[lz77_node->data.literal]++;
		} else {
			freqs_lit[257 + deflate_huffman_length_index(lz77_node->data.match.length)]++;
			freqs_dist[deflate_huffman_distance_index(lz77_node->data.match.distance)]++;
		}
	}

	/* add "end of block" character */
	freqs_lit[256]++;
}

/**
 * @brief Write huffman header (= dictionnary).
 * 
 * @param bs_out 	output bit stream
 * @param nodes_lit 	huffman literal nodes
 * @param nodes_dist	huffman distance nodes
 */
static void __write_huffman_header(struct bit_stream *bs_out, struct huff_node **nodes_lit, struct huff_node **nodes_dist)
{
	int i, n;

	/* count number of literal nodes */
	for (i = 0, n = 0; i < DYN_HUFF_NR_CODES; i++)
		if (nodes_lit[i])
			n++;

	/* write number of literal nodes */
	bit_stream_write_bits(bs_out, n, 9, BIT_ORDER_MSB);

	/* write literal dictionnary */
	for (i = 0; i < DYN_HUFF_NR_CODES; i++) {
		if (!nodes_lit[i])
			continue;

		bit_stream_write_bits(bs_out, nodes_lit[i]->val, 9, BIT_ORDER_MSB);
		bit_stream_write_bits(bs_out, nodes_lit[i]->freq, 16, BIT_ORDER_MSB);
	}

	/* count number of distance nodes */
	for (i = 0, n = 0; i < DEFLATE_HUFFMAN_NR_DISTANCE_CODES; i++)
		if (nodes_dist[i])
			n++;

	/* write number of distance nodes */
	bit_stream_write_bits(bs_out, n, 5, BIT_ORDER_MSB);

	/* write distance dictionnary */
	for (i = 0, n = 0; i < DEFLATE_HUFFMAN_NR_DISTANCE_CODES; i++) {
		if (!nodes_dist[i])
			continue;

		bit_stream_write_bits(bs_out, nodes_dist[i]->val, 5, BIT_ORDER_MSB);
		bit_stream_write_bits(bs_out, nodes_dist[i]->freq, 16, BIT_ORDER_MSB);
	}
}

/**
 * @brief Read huffman header (= dictionnary).
 * 
 * @param bs_in 	input bit stream
 * @param freqs_lit 	output literals frequencies
 * @param freqs_dist	output distances frequencies
 */
static void __read_huffman_header(struct bit_stream *bs_in, uint32_t *freqs_lit, uint32_t *freqs_dist)
{
	uint32_t val;
	int i, n;

	/* read number of literal nodes */
	n = bit_stream_read_bits(bs_in, 9, BIT_ORDER_MSB);

	/* read literal nodes */
	for (i = 0; i < n; i++) {
		val = bit_stream_read_bits(bs_in, 9, BIT_ORDER_MSB);
		freqs_lit[val] = bit_stream_read_bits(bs_in, 16, BIT_ORDER_MSB);
	}

	/* read number of distance nodes */
	n = bit_stream_read_bits(bs_in, 5, BIT_ORDER_MSB);

	/* read distance nodes */
	for (i = 0; i < n; i++) {
		val = bit_stream_read_bits(bs_in, 5, BIT_ORDER_MSB);
		freqs_dist[val] = bit_stream_read_bits(bs_in, 16, BIT_ORDER_MSB);
	}
}

/**
 * @brief Write a huffman code.
 * 
 * @param bs_out 	output bit stream
 * @param huff_node 	huffman node
 */
static void __write_huffman_code(struct bit_stream *bs_out, struct huff_node *huff_node)
{
	bit_stream_write_bits(bs_out, huff_node->huff_code, huff_node->nr_bits, BIT_ORDER_MSB);
}

/**
 * @brief Encode a block with huffman codes.
 * 
 * @param lz77_nodes	LZ77 nodes
 * @param nodes_lit 	huffman literal nodes
 * @param nodes_dist	huffman distance nodes
 * @param bs_out 	output bit stream
 */
static void __write_huffman_content(struct lz77_node *lz77_nodes, struct huff_node **nodes_lit, struct huff_node **nodes_dist, struct bit_stream *bs_out)
{
	struct lz77_node *lz77_node;

	/* for each lz77 nodes */
	for (lz77_node = lz77_nodes; lz77_node != NULL; lz77_node = lz77_node->next) {
		/* get huffman node and write it to output bit stream */
		if (lz77_node->is_literal) {
			__write_huffman_code(bs_out, nodes_lit[lz77_node->data.literal]);
		} else {
			/* write length */
			__write_huffman_code(bs_out, nodes_lit[257 + deflate_huffman_length_index(lz77_node->data.match.length)]);
			deflate_huffman_encode_length_extra_bits(bs_out, lz77_node->data.match.length);

			/* write distance */
			__write_huffman_code(bs_out, nodes_dist[deflate_huffman_distance_index(lz77_node->data.match.distance)]);
			deflate_huffman_encode_distance_extra_bits(bs_out, lz77_node->data.match.distance);
		}
	}
}

/**
 * @brief Decode a huffman node value.
 * 
 * @param bs_in 	input bit stream
 * @param root	 	huffman tree
 *
 * @return huffman value
 */
static int __read_huffman_val(struct bit_stream *bs_in, struct huff_node *root)
{
	struct huff_node *node;
	int bit;

	for (node = root;;) {
		bit = bit_stream_read_bits(bs_in, 1, BIT_ORDER_MSB);

		/* walk through the tree */
		if (bit)
			node = node->right;
		else
			node = node->left;

		if (__huffman_leaf(node))
			return node->val;
	}

	return -1;
}

/**
 * @brief Decode input buffer with huffman codes.
 * 
 * @param bs_in 	input bit stream
 * @param buf_out	output buffer
 * @param root_lit	huffman literals tree
 * @param root_dist	huffman distances tree
 */
static int __read_huffman_content(struct bit_stream *bs_in, uint8_t *buf_out, struct huff_node *root_lit, struct huff_node *root_dist)
{
	int literal, length, distance, i, n;

	/* decode each character */
	for (n = 0;;) {
		/* get next literal */
		literal = __read_huffman_val(bs_in, root_lit);

		/* end of block : exit */
		if (literal == 256)
			break;

		/* literal */
		if (literal < 256) {
			buf_out[n++] = literal;
			continue;
		}

		/* decode lz77 length and distance */
		length = deflate_huffman_decode_length(bs_in, literal - 257);
		distance = deflate_huffman_decode_distance(bs_in, __read_huffman_val(bs_in, root_dist));

		/* duplicate pattern */
		for (i = 0; i < length; i++, n++)
			buf_out[n] = buf_out[n - distance];
	}

	return n;
}

/**
 * @brief Compress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param lz77_nodes 		LZ77 nodes
 * @param bs_out 		output bit stream
 */
void deflate_dyn_huffman_compress(struct lz77_node *lz77_nodes, struct bit_stream *bs_out)
{
	uint32_t freqs_lit[DYN_HUFF_NR_CODES] = { 0 }, freqs_dist[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = { 0 };
	struct huff_node *huff_nodes_dist[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = { NULL };
	struct huff_node *huff_nodes_lit[DYN_HUFF_NR_CODES] = { NULL };
	struct huff_node *huff_tree_lit, *huff_tree_dist;

	/* compute literals and distances frequencies */
	__compute_frequencies(lz77_nodes, freqs_lit, freqs_dist);

	/* build huffman trees */
	huff_tree_lit = huffman_tree_create(freqs_lit, DYN_HUFF_NR_CODES);
	huff_tree_dist = huffman_tree_create(freqs_dist, DEFLATE_HUFFMAN_NR_DISTANCE_CODES);

	/* extract huffman nodes */
	huffman_tree_extract_nodes(huff_tree_lit, huff_nodes_lit);
	huffman_tree_extract_nodes(huff_tree_dist, huff_nodes_dist);

	/* write huffman header (= write dictionnary with frequencies) */
	__write_huffman_header(bs_out, huff_nodes_lit, huff_nodes_dist);

	/* write huffman content (= encode lz77 data) */
	__write_huffman_content(lz77_nodes, huff_nodes_lit, huff_nodes_dist, bs_out);

	/* write end of block */
	__write_huffman_code(bs_out, huff_nodes_lit[256]);

	/* free huffman trees */
	huffman_tree_free(huff_tree_lit);
	huffman_tree_free(huff_tree_dist);
}

/**
 * @brief Uncompress LZ77 nodes with dynamic huffman alphabet.
 * 
 * @param bs_in 		input bit stream
 * @param buf_out 		output buffer
 *
 * @return number of bytes written to output buffer
 */
int deflate_dyn_huffman_uncompress(struct bit_stream *bs_in, uint8_t *buf_out)
{
	uint32_t freqs_lit[DYN_HUFF_NR_CODES] = { 0 }, freqs_dist[DEFLATE_HUFFMAN_NR_DISTANCE_CODES] = { 0 };
	struct huff_node *huff_tree_lit, *huff_tree_dist;
	int len;

	/* read dynamic huffman header (= get frequencies and dictionnary ) */
	__read_huffman_header(bs_in, freqs_lit, freqs_dist);

	/* build huffman trees */
	huff_tree_lit = huffman_tree_create(freqs_lit, DYN_HUFF_NR_CODES);
	huff_tree_dist = huffman_tree_create(freqs_dist, DEFLATE_HUFFMAN_NR_DISTANCE_CODES);

	/* read/decode huffman content */
	len = __read_huffman_content(bs_in, buf_out, huff_tree_lit, huff_tree_dist);

	/* free huffman trees */
	huffman_tree_free(huff_tree_lit);
	huffman_tree_free(huff_tree_dist);

	return len;
}
