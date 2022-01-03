#ifndef _MEM_H_
#define _MEM_H_

#include <stdio.h>

void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
void xfree(void *ptr);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);

#endif
