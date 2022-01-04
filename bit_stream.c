#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bit_stream.h"
#include "mem.h"

/*
 * Create a bit stream (capacity in byte).
 */
struct bit_stream_t *bit_stream_create(size_t capacity)
{
  struct bit_stream_t *bs;

  bs = (struct bit_stream_t *) xmalloc(sizeof(struct bit_stream_t));
  bs->buf = (unsigned char *) xmalloc(capacity);
  bs->byte_offset = 0;
  bs->bit_offset = 0;
  memset(bs->buf, 0, capacity);

  return bs;
}

/*
 * Free a bit stream.
 */
void bit_stream_free(struct bit_stream_t *bs)
{
  if (!bs)
    return;

  xfree(bs->buf);
  free(bs);
}

/*
 * Flush a bit stream to output file.
 */
void bit_stream_flush(struct bit_stream_t *bs, FILE *fp, int flush_last_byte)
{
  int last_bits, nb_last_bits;

  /* write all bytes */
  if (bs->byte_offset > 0)
    fwrite(bs->buf, sizeof(char), bs->byte_offset, fp);

  /* write last byte */
  if (flush_last_byte && bs->bit_offset > 0) {
    nb_last_bits = bs->bit_offset;
    bs->bit_offset = 0;
    last_bits = bit_stream_read_bits(bs, nb_last_bits);
    fwrite(&last_bits, sizeof(char), 1, fp);
    goto out;
  }

  /* else copy last bits at the beginning of the stream */
  if (bs->bit_offset > 0)
    bs->buf[0] = bs->buf[bs->byte_offset];

out:
  /* reset bit stream (do no reset bit offset !) */
  bs->byte_offset = 0;
}

/*
 * Write a single bit.
 */
void bit_stream_write_bit(struct bit_stream_t *bs, int value)
{
  if (bs->bit_offset == 0) {
    bs->buf[bs->byte_offset] = value << 7;
    bs->bit_offset = 1;
    return;
  }

  bs->buf[bs->byte_offset] |= value << (8 - bs->bit_offset - 1);

  if (bs->bit_offset == 7) {
    bs->byte_offset++;
    bs->bit_offset = 0;
  } else {
    bs->bit_offset++;
  }
}

/*
 * Write bits.
 */
void bit_stream_write_bits(struct bit_stream_t *bs, int value, int nb_bits)
{
  int first_byte_bits, last_byte_bits, full_bytes, i;

  /* check number of bits */
  if (nb_bits <= 0)
    return;

  /* write first byte */
  first_byte_bits = 8 - bs->bit_offset;
  if (first_byte_bits != 8) {
    if (nb_bits < first_byte_bits) {
      bs->buf[bs->byte_offset] |= value << (first_byte_bits - nb_bits);
      bs->bit_offset += nb_bits;
    } else {
      bs->buf[bs->byte_offset] |= value >> (nb_bits - first_byte_bits);
      bs->byte_offset++;
      bs->bit_offset = 0;
    }

    nb_bits -= first_byte_bits;

    if (nb_bits <= 0)
      return;
  }

  /* write last byte */
  last_byte_bits = nb_bits % 8;
  full_bytes = nb_bits / 8;
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

/*
 * Read a single bit.
 */
int bit_stream_read_bit(struct bit_stream_t *bs)
{
  int value;

  if (bs->bit_offset == 0) {
    value = bs->buf[bs->byte_offset] >> 7;
    bs->bit_offset = 1;
    return value;
  }

  value = (bs->buf[bs->byte_offset] >> (8 - bs->bit_offset - 1)) & 0x1;

  if (bs->bit_offset == 7) {
    bs->byte_offset++;
    bs->bit_offset = 0;
  } else {
    bs->bit_offset++;
  }

  return value;
}

/*
 * Read bits.
 */
int bit_stream_read_bits(struct bit_stream_t *bs, int nb_bits)
{
  int first_byte_bits, last_byte_bits, full_bytes, value, i;

  /* check number of bits */
  if (nb_bits <= 0)
    return 0;

  /* read first byte */
  first_byte_bits = 8 - bs->bit_offset;
  if (first_byte_bits != 8) {
    if (nb_bits < first_byte_bits) {
      value = bs->buf[bs->byte_offset] >> (first_byte_bits - nb_bits);
      value &= (1 << nb_bits) - 1;
      bs->bit_offset += nb_bits;
    } else {
      value = bs->buf[bs->byte_offset] & ((1 << first_byte_bits) - 1);
      bs->byte_offset++;
      bs->bit_offset = 0;
    }

    nb_bits -= first_byte_bits;

    if (nb_bits <= 0)
      return value;
  } else {
    value = 0;
  }

  /* copy middle bytes */
  full_bytes = nb_bits / 8;
  for (i = 0; i < full_bytes; i++) {
    value <<= 8;
    value |= bs->buf[bs->byte_offset + i];
  }


  /* read last byte */
  last_byte_bits = nb_bits % 8;
  if (last_byte_bits != 0) {
    value <<= last_byte_bits;
    value |= bs->buf[bs->byte_offset + full_bytes] >> (8 - last_byte_bits);
    bs->bit_offset = last_byte_bits;
  }

  /* update byte offset */
  bs->byte_offset += full_bytes;

  return value;
}
