#include <stdio.h>
#include <stdlib.h>

#include "lz77.h"
#include "mem.h"

#define HASH_SIZE       (1 << 10)
#define MAX_DISTANCE    (1 << 15)

/*
 * Hash table node.
 */
struct hash_node_t {
  int index;
  struct hash_node_t *next;
};

/*
 * Add a node to a hash table.
 */
static struct hash_node_t *hash_add_node(int index, struct hash_node_t *head)
{
  struct hash_node_t *node;

  node = (struct hash_node_t *) xmalloc(sizeof(struct hash_node_t));
  node->index = index;
  node->next = head;

  return node;
}

/*
 * Free a hash table.
 */
static void hash_node_free(struct hash_node_t *head)
{
  if (head->next)
    hash_node_free(head->next);

  free(head);
}

/*
 * Free a hash table.
 */
static void hash_table_free(struct hash_node_t **hash_table, int size)
{
  int i;

  for (i = 0; i < size; i++)
    if (hash_table[i])
      hash_node_free(hash_table[i]);

  free(hash_table);
}

/*
 * Hash 3 characters.
 */
static inline int hash(unsigned char a, unsigned char b, unsigned char c)
{
  int h = a;
  h = (h << 5) - h + b;
  h = (h << 5) - h + c;
  return h % HASH_SIZE;
}

/*
 * Create a LZ77 literal node.
 */
static struct lz77_node_t *lz77_create_literal_node(unsigned char c)
{
  struct lz77_node_t *node;

  node = (struct lz77_node_t *) xmalloc(sizeof(struct lz77_node_t));
  node->is_literal = 1;
  node->data.literal = c;
  node->next = NULL;

  return node;
}

/*
 * Create a lZ77 match node.
 */
static struct lz77_node_t *lz77_create_match_node(int distance, int length)
{
  struct lz77_node_t *node;

  node = (struct lz77_node_t *) xmalloc(sizeof(struct lz77_node_t));
  node->is_literal = 0;
  node->data.match.distance = distance;
  node->data.match.length = length;
  node->next = NULL;

  return node;
}

/*
 * Find best matching pattern.
 */
static struct lz77_node_t *lz77_best_match(struct hash_node_t *match, char *buf, int len, char *ptr)
{
  struct hash_node_t *match_max;
  int i, len_max = 0, max;
  char *match_buf;

  /* compute maximum match length */
  max = len - (ptr - buf);
  if (max > 256)
    max = 256;

  /* for each match */
  for (; match != NULL; match = match->next) {
    match_buf = buf + match->index;

    /* match too far */
    if (ptr - match_buf > MAX_DISTANCE)
      break;

    /* no way to improve best match */
    if (match_buf[len_max] != ptr[len_max])
      continue;

    /* compute match length */
    for (i = 0; match_buf[i] == ptr[i] && i < max; i++);

    /* update maximum match length */
    if (i > len_max) {
      len_max = i;
      match_max = match;
    }
  }

  /* match too short : create a literal */
  if (len_max < 3)
    return lz77_create_literal_node(*ptr);

  /* create a match node */
  return lz77_create_match_node(ptr - (buf + match_max->index), len_max);
}

/*
 * Skip 'len' bytes (and hash skipped bytes).
 */
static int lz77_skip(char *buf, struct hash_node_t **hash_table, char *ptr, int len)
{
  int i, index;

  /* hash skipped bytes */
  for (i = 0; i < len; i++) {
    index = hash(ptr[i], ptr[i + 1], ptr[i + 2]);
    hash_table[index] = hash_add_node(ptr + i - buf, hash_table[index]);
  }

  return len;
}

/*
 * Compress a buffer with LZ77 algorithm.
 */
struct lz77_node_t *lz77_compress(char *buf, int len)
{
  struct lz77_node_t *lz77_head = NULL, *lz77_tail = NULL;
  struct hash_node_t **hash_table, *match;
  struct lz77_node_t *node;
  int i, index;
  char *ptr;

  /* create hash table */
  hash_table = (struct hash_node_t **) xmalloc(sizeof(struct hash_node_t *) * HASH_SIZE);
  for (i = 0; i < HASH_SIZE; i++)
    hash_table[i] = NULL;

  /* find matching patterns */
  for (ptr = buf; ptr + 2 - buf < len; ptr++) {
    /* compute next 3 characters hash */
    index = hash(ptr[0], ptr[1], ptr[2]);

    /* add it to hash table */
    match = hash_table[index];
    hash_table[index] = hash_add_node(ptr - buf, match);

    /* find best match */
    node = lz77_best_match(match, buf, len, ptr);

    /* add node to list */
    if (!lz77_head) {
      lz77_head = node;
      lz77_tail = node;
    } else {
      lz77_tail->next = node;
      lz77_tail = node;
    }

    /* skip match */
    if (node->is_literal == 0)
      ptr += lz77_skip(buf, hash_table, ptr, node->data.match.length - 1);
  }

  /* add remaining bytes */
  for (; ptr - buf < len; ptr++) {
    /* create a literal node */
    node = lz77_create_literal_node(*ptr);

    /* add node to list */
    if (!lz77_head) {
      lz77_head = node;
      lz77_tail = node;
    } else {
      lz77_tail->next = node;
      lz77_tail = node;
    }
  }

  /* free hash table */
  hash_table_free(hash_table, HASH_SIZE);

  /* return lz77 nodes */
  return lz77_head;
}

/*
 * Free LZ77 nodes.
 */
void lz77_free_nodes(struct lz77_node_t *node)
{
  if (!node)
    return;

  lz77_free_nodes(node->next);
  free(node);
}
