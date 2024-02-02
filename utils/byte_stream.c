#include <stdio.h>
#include <string.h>

#include "byte_stream.h"
#include "mem.h"

#define MIN_GROW_SIZE		64
#define MAX(x, y)		((x) > (y) ? (x) : (y))

/**
 * @brief Grow a byte stream.
 * 
 * @param bs 		bit stream
 * @param nr_bytes	number of bytes to grow
 */
static void __byte_stream_grow(struct byte_stream *bs, uint32_t nr_bytes)
{
	bs->capacity += MAX(nr_bytes, MIN_GROW_SIZE);
	bs->buf = (uint8_t *) xrealloc(bs->buf, bs->capacity);
}

/**
 * @brief Write bytes.
 * 
 * @param bs 		byte stream
 * @param value 	value
 * @param nr_bytes	number of bytes to write
 */
void byte_stream_write(struct byte_stream *bs, uint8_t *value, uint32_t nr_bytes)
{
	/* grow byte stream if needed */
	if (bs->size + nr_bytes > bs->capacity)
		__byte_stream_grow(bs, bs->size + nr_bytes - bs->capacity);

	/* copy data */
	memcpy(bs->buf + bs->size, value, nr_bytes);
	bs->size += nr_bytes;
}

/**
 * @brief Write uint8_t.
 * 
 * @param bs 		byte stream
 * @param value 	value
 */
void byte_stream_write_u8(struct byte_stream *bs, uint8_t value)
{
	/* grow byte stream if needed */
	if (bs->size + sizeof(uint8_t) > bs->capacity)
		__byte_stream_grow(bs, bs->size + sizeof(uint8_t) - bs->capacity);

	/* copy data */
	bs->buf[bs->size++] = value;
}

/**
 * @brief Write uint32_t.
 * 
 * @param bs 		byte stream
 * @param value 	value
 */
void byte_stream_write_u32(struct byte_stream *bs, uint32_t value)
{
	/* grow byte stream if needed */
	if (bs->size + sizeof(uint32_t) > bs->capacity)
		__byte_stream_grow(bs, bs->size + sizeof(uint32_t) - bs->capacity);

	/* copy data */
	*((uint32_t *) (bs->buf + bs->size)) = value;
	bs->size += sizeof(uint32_t);
}