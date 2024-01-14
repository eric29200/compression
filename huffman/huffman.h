#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#include <stdio.h>
#include <stdint.h>

/**
 * @brief Compress a buffer with huffman algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *huffman_compress(uint8_t *src, uint32_t src_len, uint32_t *dst_len);

/**
 * @brief Uncompress a buffer with huffman algorithm.
 * 
 * @param src 		input buffer
 * @param src_len 	input buffer length
 * @param dst_len 	output buffer length
 *
 * @return output buffer
 */
uint8_t *huffman_uncompress(uint8_t *src, uint32_t src_len, uint32_t *dst_len);

#endif