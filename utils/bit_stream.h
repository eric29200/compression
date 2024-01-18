#ifndef _BIT_STREAM_H_
#define _BIT_STREAM_H_

#include <stdio.h>
#include <stdint.h>

/**
 * @brief Bit stream.
 */
struct bit_stream {
	uint8_t *		buf;			/* data */
	uint32_t 		capacity;		/* capacity */
	uint32_t 		byte_offset;		/* current byte position */
	uint32_t 		bit_offset;		/* current bit position (in last byte) */
};

/**
 * @brief Write bits.
 * 
 * @param bs 		bit stream
 * @param value 	value
 * @param nr_bits	number of bits to write
 */
void bit_stream_write_bits(struct bit_stream *bs, uint32_t value, int nr_bits);

/**
 * @brief Read bits.
 * 
 * @param bs 		bit stream
 * @param nr_bits 	number of bits to read
 * 
 * @return value
 */
uint32_t bit_stream_read_bits(struct bit_stream *bs, int nr_bits);

#endif
