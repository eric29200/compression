#include <stdio.h>

struct heap_t {
  int type;
  void **data;
  int capacity;
  int size;
  int (*compare_func)(const void *, const void *);
};

#define HEAP_MIN                1
#define HEAP_MAX                2

#define heap_is_full(heap)      ((heap)->size >= (heap)->capacity)
#define heap_parent(i)          ((i) == 0 ? -1 : ((((i) + 1) / 2) - 1))
#define heap_left(i)            (2 * (i) + 1)
#define heap_right(i)           (2 * (i) + 2)

struct heap_t *heap_create(int type, int capacity, int (*compare_func)(const void *, const void *));
void heap_free(struct heap_t *heap);
void heap_free_full(struct heap_t *heap, void (*free_func)(void *));
void heap_insert(struct heap_t *heap, void *data);
void *heap_min(struct heap_t *heap);
void *heap_max(struct heap_t *heap);
