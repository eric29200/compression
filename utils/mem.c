#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mem.h"

/*
 * Malloc or exit.
 */
void *xmalloc(size_t size)
{
  void *ptr;

  ptr = malloc(size);
  if (!ptr)
    exit(2);

  return ptr;
}

/*
 * Realloc or exit.
 */
void *xrealloc(void *ptr, size_t size)
{
  ptr = realloc(ptr, size);
  if (!ptr)
    exit(2);

  return ptr;
}

/*
 * Safe free.
 */
void xfree(void *ptr)
{
  if (ptr)
    free(ptr);
}

/*
 * Strdup or exit.
 */
char *xstrdup(const char *s)
{
  char *sc;

  sc = strdup(s);
  if (!sc)
    exit(2);

  return sc;
}

/*
 * Strndup or exit.
 */
char *xstrndup(const char *s, size_t n)
{
  size_t len = strnlen(s, n);
  char *new = (char *) xmalloc(len + 1);
  new[len] = '\0';
  return (char *) memcpy(new, s, len);
}
