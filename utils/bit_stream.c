#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bit_stream.h"
#include "mem.h"

#define GROW_SIZE		64

#define read_bit(bs, i)		(((bs)->buf[(bs)->byte_offset] >> (i)) & 0x01)
#define set_bit(bs, i)		((bs)->buf[(bs)->byte_offset] |= 0x01 << (i))
#define clear_bit(bs, i)	((bs)->buf[(bs)->byte_offset] &= ~(0x01 << (i)))

/**
 * @brief Grow a bit stream.
 * 
 * @param bs 	bit stream
 */
static void __bit_stream_grow(struct bit_stream *bs)
{
	bs->capacity = bs->byte_offset + GROW_SIZE;
	bs->buf = (uint8_t *) xrealloc(bs->buf, bs->capacity);
}

/**
 * @brief Write bits.
 * 
 * @param bs 		bit stream
 * @param value 	value
 * @param nr_bits	number of bits to write
 * @param bit_order	bit order (Least Significant first or Most Significant first)
 */
void bit_stream_write_bits(struct bit_stream *bs, uint32_t value, int nr_bits, int bit_order)
{
	int start, end, step, i;

	/* number of bits must be <= 32 */
	assert(nr_bits <= 32);

	/* bit offset must be < 8 */
	assert(bs->bit_offset < 8);

	/* check bit order */
	assert(bit_order == BIT_ORDER_LSB || bit_order == BIT_ORDER_MSB);

	/* check number of bits */
	if (nr_bits <= 0)
		return;

	/* choose bit order */
	if (bit_order == BIT_ORDER_LSB) {
		start = 0;
		end = nr_bits;
		step = 1;
	} else {
		start = nr_bits - 1;
		end = -1;
		step = -1;
	}

	/* write each bit */
	for (i = start; i != end; i += step) {
		/* grow bit stream if needed */
		if (bs->byte_offset >= bs->capacity)
			__bit_stream_grow(bs);

		/* write next bit */
		if ((value >> i) & 0x01)
			set_bit(bs, bs->bit_offset);
		else
			clear_bit(bs, bs->bit_offset);

		/* go to next byte if needed */
		if (++bs->bit_offset == 8) {
			bs->byte_offset++;
			bs->bit_offset = 0;
		}
	}
}

/**
 * @brief Read bits.
 * 
 * @param bs 		bit stream
 * @param nr_bits 	number of bits to read
 * @param bit_order	bit order (Least Significant first or Most Significant first)
 * 
 * @return value
 */
uint32_t bit_stream_read_bits(struct bit_stream *bs, int nr_bits, int bit_order)
{
	int start, end, step, i;
	uint32_t value = 0;

	/* number of bits must be <= 32 */
	assert(nr_bits <= 32);

	/* bit offset must be < 8 */
	assert(bs->bit_offset < 8);

	/* check bit order */
	assert(bit_order == BIT_ORDER_LSB || bit_order == BIT_ORDER_MSB);
	
	/* check number of bits */
	if (nr_bits <= 0)
		return value;

	/* choose bit order */
	if (bit_order == BIT_ORDER_LSB) {
		start = 0;
		end = nr_bits;
		step = 1;
	} else {
		start = nr_bits - 1;
		end = -1;
		step = -1;
	}

	/* read each bit */
	for (i = start; i != end; i += step) {
		/* read next bit */
		value |= read_bit(bs, bs->bit_offset) << i;

		/* go to next byte if needed */
		if (++bs->bit_offset == 8) {
			bs->byte_offset++;
			bs->bit_offset = 0;
		}
	}

	return value;
}

/**
 * @brief Flush last byte.
 * 
 * @param bs 		bit stream
 */
void bit_stream_flush(struct bit_stream *bs)
{
	if (bs->bit_offset) {
		bs->byte_offset++;
		bs->bit_offset = 0;
	}
}
