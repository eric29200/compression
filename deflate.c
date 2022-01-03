#include <stdio.h>

#include "lz77.h"

#include "mem.h"

#define INPUT_FILE      "/home/eric/test.txt"
#define BLOCK_SIZE      0xFFFF

int main()
{
  struct lz77_node_t *lz77_nodes;
  char *block;
  FILE *fp;
  int len;

  /* open input file */
  fp = fopen(INPUT_FILE, "r");
  if (!fp) {
    perror("fopen");
    return -1;
  }

  /* allocate block buffer */
  block = (char *) xmalloc(BLOCK_SIZE);

  /* compress block by block */
  for (;;) {
    /* read next block */
    len = fread(block, 1, BLOCK_SIZE, fp);
    if (len <= 0)
      break;

    /* LZ77 compression */
    lz77_nodes = lz77_compress(block, len);

    /* free lz77 nodes */
    lz77_free_nodes(lz77_nodes);

    /* end of file : exit */
    if (len != BLOCK_SIZE)
      break;
  }

  /* close input file */
  fclose(fp);

  return 0;
}
