#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bit_stream.h"
#include "../utils/mem.h"

#define GROW_SIZE		64

/**
 * @brief Grow a bit stream.
 * 
 * @param bs 	bit stream
 */
static void __bit_stream_grow(struct bit_stream *bs)
{
	bs->capacity += GROW_SIZE;
	bs->buf = (uint8_t *) xrealloc(bs->buf, bs->capacity);
}

/**
 * @brief Write bits.
 * 
 * @param bs 		bit stream
 * @param value 	value
 * @param nr_bits	number of bits to write
 */
void bit_stream_write_bits(struct bit_stream *bs, int value, int nr_bits)
{
	int first_byte_bits, last_byte_bits, full_bytes, i;

	/* number of bits must be <= 32 */
	assert(nr_bits <= 32);

	/* check number of bits */
	if (nr_bits <= 0)
		return;

	/* check need to grow bit stream */
	if (bs->byte_offset + nr_bits / 8 >= bs->capacity)
		__bit_stream_grow(bs);

	/* write first byte */
	first_byte_bits = 8 - bs->bit_offset;
	if (first_byte_bits != 8) {
		if (nr_bits < first_byte_bits) {
			bs->buf[bs->byte_offset] |= value << (first_byte_bits - nr_bits);
			bs->bit_offset += nr_bits;
		} else {
			bs->buf[bs->byte_offset] |= value >> (nr_bits - first_byte_bits);
			bs->byte_offset++;
			bs->bit_offset = 0;
		}

		nr_bits -= first_byte_bits;

		if (nr_bits <= 0)
			return;
	}

	/* write last byte */
	last_byte_bits = nr_bits % 8;
	full_bytes = nr_bits / 8;
	if (last_byte_bits != 0) {
		bs->buf[bs->byte_offset + full_bytes] = value << (8 - last_byte_bits);
		value >>= last_byte_bits;
		bs->bit_offset = last_byte_bits;
	}

	/* write middle bytes */
	for (i = full_bytes; i > 0; i--) {
		bs->buf[bs->byte_offset + i - 1] = value;
		value >>= 8;
	}

	/* update byte offset */
	bs->byte_offset += full_bytes;
}

/**
 * @brief Read bits.
 * 
 * @param bs 		bit stream
 * @param nr_bits 	number of bits to read
 * 
 * @return value
 */
int bit_stream_read_bits(struct bit_stream *bs, int nr_bits)
{
	int first_byte_bits, last_byte_bits, full_bytes, value, i;

	/* check number of bits */
	if (nr_bits <= 0)
		return 0;

	/* read first byte */
	first_byte_bits = 8 - bs->bit_offset;
	if (first_byte_bits != 8) {
		if (nr_bits < first_byte_bits) {
			value = bs->buf[bs->byte_offset] >> (first_byte_bits - nr_bits);
			value &= (1 << nr_bits) - 1;
			bs->bit_offset += nr_bits;
		} else {
			value = bs->buf[bs->byte_offset] & ((1 << first_byte_bits) - 1);
			bs->byte_offset++;
			bs->bit_offset = 0;
		}

		nr_bits -= first_byte_bits;

		if (nr_bits <= 0)
			return value;
	} else {
		value = 0;
	}

	/* copy middle bytes */
	full_bytes = nr_bits / 8;
	for (i = 0; i < full_bytes; i++) {
		value <<= 8;
		value |= bs->buf[bs->byte_offset + i];
	}


	/* read last byte */
	last_byte_bits = nr_bits % 8;
	if (last_byte_bits != 0) {
		value <<= last_byte_bits;
		value |= bs->buf[bs->byte_offset + full_bytes] >> (8 - last_byte_bits);
		bs->bit_offset = last_byte_bits;
	}

	/* update byte offset */
	bs->byte_offset += full_bytes;

	return value;
}

/**
 * @brief Flush a bit stream to a buffer.
 * 
 * @param bs 			bit stream
 * @param buf 			output buffer
 * @param flush_last_byte 	flush last byte ?
 *
 * @return number of bytes written
 */
size_t bit_stream_flush(struct bit_stream *bs, uint8_t *buf, int flush_last_byte)
{
	uint8_t last_bits;
	int nr_last_bits;
	size_t n = 0;

	/* write all bytes */
	if (bs->byte_offset > 0) {
		memcpy(buf, bs->buf, bs->byte_offset);
		n = bs->byte_offset;
	}

	/* write last byte */
	if (flush_last_byte && bs->bit_offset > 0) {
		nr_last_bits = bs->bit_offset;
		bs->bit_offset = 0;
		last_bits = bit_stream_read_bits(bs, nr_last_bits) << (8 - nr_last_bits);
		buf[n++] = last_bits;
		goto out;
	}

	/* else copy last bits at the beginning of the stream */
	if (bs->bit_offset > 0)
		bs->buf[0] = bs->buf[bs->byte_offset];

out:
	/* reset bit stream (do no reset bit offset !) */
	bs->byte_offset = 0;

	return n;
}
