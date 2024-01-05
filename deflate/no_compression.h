#ifndef _NO_COMPRESSION_H_
#define _NO_COMPRESSION_H_

#include <stdint.h>

#include "../utils/bit_stream.h"

/**
 * @brief Compress a block.
 * 
 * @param block 	input block
 * @param len 		block length
 * @param last_block 	is this last block ?
 * @param bs_out 	output bit stream
 */
void deflate_no_compression_compress(uint8_t *block, int len, int last_block, struct bit_stream *bs_out);

/**
 * @brief Uncompress a block.
 * 
 * @param bs_in 	input bit stream
 * @param buf_out 	output buffer
 * 
 * @return block length
 */
int deflate_no_compression_uncompress(struct bit_stream *bs_in, uint8_t *buf_out);

#endif
