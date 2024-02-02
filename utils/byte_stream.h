#ifndef _BYTE_STREAM_H_
#define _BYTE_STREAM_H_

#include <stdio.h>
#include <stdint.h>

/**
 * @brief Byte stream.
 */
struct byte_stream {
	uint8_t *		buf;			/* data */
	uint32_t 		capacity;		/* capacity */
	uint32_t		size;			/* size */
};

/**
 * @brief Write bytes.
 * 
 * @param bs 		byte stream
 * @param value 	value
 * @param nr_bytes	number of bytes to write
 */
void byte_stream_write(struct byte_stream *bs, uint8_t *value, uint32_t nr_bytes);

/**
 * @brief Write uint8_t.
 * 
 * @param bs 		byte stream
 * @param value 	value
 */
void byte_stream_write_u8(struct byte_stream *bs, uint8_t value);

/**
 * @brief Write uint32_t.
 * 
 * @param bs 		byte stream
 * @param value 	value
 */
void byte_stream_write_u32(struct byte_stream *bs, uint32_t value);

#endif
