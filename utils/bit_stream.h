#ifndef _BIT_STREAM_H_
#define _BIT_STREAM_H_

#include <stdio.h>
#include <stdint.h>

#define BIT_ORDER_MSB		1
#define BIT_ORDER_LSB		2

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
 * @param bit_order	bit order (Least Significant first or Most Significant first)
 */
void bit_stream_write_bits(struct bit_stream *bs, uint32_t value, int nr_bits, int bit_order);

/**
 * @brief Read bits.
 * 
 * @param bs 		bit stream
 * @param nr_bits 	number of bits to read
 * @param bit_order	bit order (Least Significant first or Most Significant first)
 * 
 * @return value
 */
uint32_t bit_stream_read_bits(struct bit_stream *bs, int nr_bits, int bit_order);

/**
 * @brief Flush last byte.
 * 
 * @param bs 		bit stream
 */
void bit_stream_flush(struct bit_stream *bs);

#endif
