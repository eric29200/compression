#ifndef _DEFLATE_H_
#define _DEFLATE_H_

#include <stdio.h>

#define DEFLATE_BLOCK_SIZE      0xFFFF

void deflate_compress(FILE *fp_in, FILE *fp_out);

#endif
