#ifndef _NO_COMPRESSION_H_
#define _NO_COMPRESSION_H_

#include "../utils/bit_stream.h"

void no_compression_compress(char *block, int len, int last_block, struct bit_stream_t *bs_out);
int no_compression_uncompress(struct bit_stream_t *bs_in, char *buf_out);

#endif
