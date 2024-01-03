#ifndef _FIX_HUFFMAN_H_
#define _FIX_HUFFMAN_H_

#include "lz77.h"
#include "../utils/bit_stream.h"

void fix_huffman_compress(struct lz77_node_t *lz77_nodes, int last_block, struct bit_stream_t *bs_out);
int fix_huffman_uncompress(struct bit_stream_t *bs_in, unsigned char *buf_out);

#endif
