#ifndef _BIT_STREAM_H_
#define _BIT_STREAM_H_

#include <stdio.h>

struct bit_stream_t {
	unsigned char *		buf;
	size_t 			capacity;
	int 			byte_offset;
	int 			bit_offset;
};

struct bit_stream_t *bit_stream_create(size_t capacity);
void bit_stream_free(struct bit_stream_t *bs);
void bit_stream_flush(struct bit_stream_t *bs, FILE *fp, int flush_last_byte);
void bit_stream_read(struct bit_stream_t *bs, FILE *fp, int full_read);
void bit_stream_write_bit(struct bit_stream_t *bs, int value);
void bit_stream_write_bits(struct bit_stream_t *bs, int value, int nb_bits);
int bit_stream_read_bit(struct bit_stream_t *bs);
int bit_stream_read_bits(struct bit_stream_t *bs, int nb_bits);

#endif
