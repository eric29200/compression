#ifndef _BIT_STREAM_H_
#define _BIT_STREAM_H_

#include <stdio.h>
#include <stdint.h>

/**
 * @brief Bit stream.
 */
struct bit_stream {
	uint8_t *		buf;			/* data */
	size_t 			capacity;		/* capacity */
	size_t 			byte_offset;		/* current byte position */
	size_t 			bit_offset;		/* current bit position (in last byte) */
};

/**
 * @brief Write bits.
 * 
 * @param bs 		bit stream
 * @param value 	value
 * @param nr_bits	number of bits to write
 */
void bit_stream_write_bits(struct bit_stream *bs, int value, int nr_bits);

/**
 * @brief Read bits.
 * 
 * @param bs 		bit stream
 * @param nr_bits 	number of bits to read
 * 
 * @return value
 */
int bit_stream_read_bits(struct bit_stream *bs, int nr_bits);

/**
 * @brief Flush a bit stream to a buffer.
 * 
 * @param bs 			bit stream
 * @param buf 			output buffer
 * @param flush_last_byte 	flush last byte ?
 *
 * @return number of bytes written
 */
size_t bit_stream_flush(struct bit_stream *bs, uint8_t *buf, int flush_last_byte);

#endif
