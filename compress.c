#include <stdio.h>

#include "deflate.h"

#define INPUT_FILE      "/home/eric/test.txt"
#define OUTPUT_FILE     "/home/eric/test.txt.deflate"

int main()
{
  FILE *fp_in, *fp_out;

  /* open input file */
  fp_in = fopen(INPUT_FILE, "r");
  if (!fp_in) {
    perror("fopen");
    return -1;
  }

  /* open output file */
  fp_out = fopen(OUTPUT_FILE, "w");
  if (!fp_out) {
    perror("fopen");
    fclose(fp_in);
    return -1;
  }

  /* compress input file */
  deflate_compress(fp_in, fp_out);

  /* close input/output files */
  fclose(fp_in);
  fclose(fp_out);

  return 0;
}
