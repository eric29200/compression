#include <stdio.h>
#include <string.h>
#include <time.h>

#include "deflate/deflate.h"

#define MAX_FILENAME_LEN        4096

/*
 * Usage.
 */
static void usage(char *app_name)
{
  fprintf(stderr, "%s input_file\n", app_name);
}

/*
 * Execute a compression/decompression test.
 */
static int compression_test(const char *input_file, const char *method, void (*compression)(FILE *, FILE *),
                            void (*uncompression)(FILE *, FILE *))
{
  off_t input_size, output_size;
  FILE *fp_in, *fp_out;
  clock_t start, end;
  double tc, tu;

  /* open input file */
  fp_in = fopen(input_file, "r");
  if (!fp_in) {
    perror("fopen");
    return -1;
  }

  /* open output file */
  fp_out = tmpfile();
  if (!fp_out) {
    perror("tmpfile");
    fclose(fp_in);
    return -1;
  }

  /* compress */
  start = clock();
  compression(fp_in, fp_out);
  end = clock();
  tc = (double) (end - start) / CLOCKS_PER_SEC;

  /* get files size (to compute ratio) */
  input_size = ftell(fp_in);
  output_size = ftell(fp_out);

  /* input file = output file */
  fclose(fp_in);
  fp_in = fp_out;
  rewind(fp_in);

  /* open new output file */
  fp_out = tmpfile();
  if (!fp_out) {
    perror("tmpfile");
    fclose(fp_in);
    return -1;
  }

  /* uncompress */
  start = clock();
  uncompression(fp_in, fp_out);
  end = clock();
  tu = (double) (end - start) / CLOCKS_PER_SEC;

  /* print statistics */
  printf("**************** %s ****************\n", method);
  printf("Ratio : %f\n", (double) input_size / (double) output_size);
  printf("Compression time : %f sec\n", tc);
  printf("Uncompression time : %f sec\n", tu);

  /* close input/output files */
  fclose(fp_in);
  fclose(fp_out);

  return 0;
}

/*
 * Main.
 */
int main(int argc, char **argv)
{
  /* check parameters */
  if (argc != 2) {
    usage(argv[0]);
    return -1;
  }

  /* deflate compression */
  return compression_test(argv[1], "Deflate", deflate_compress, deflate_uncompress);
}
