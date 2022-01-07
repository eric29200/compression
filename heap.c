#include <stdio.h>
#include <stdlib.h>

#include "heap.h"
#include "mem.h"

/*
 * Create a heap.
 */
struct heap_t *heap_create(int type, int capacity, int (*compare_func)(const void *, const void *))
{
  struct heap_t *heap;

  /* check type */
  if (type != HEAP_MIN && type != HEAP_MAX)
    return NULL;

  heap = (struct heap_t *) xmalloc(sizeof(struct heap_t));
  heap->type = type;
  heap->capacity = capacity;
  heap->size = 0;
  heap->compare_func = compare_func;
  heap->data = (void **) xmalloc(sizeof(void *) * capacity);

  return heap;
}

/*
 * Free a heap.
 */
void heap_free(struct heap_t *heap)
{
  if (heap) {
    xfree(heap->data);
    free(heap);
  }
}

/*
 * Free a heap.
 */
void heap_free_full(struct heap_t *heap, void (*free_func)(void *))
{
  int i;

  if (heap) {
    if (heap->data) {
      for (i = 0; i < heap->capacity; i++)
        if (heap->data[i])
          free_func(heap->data[i]);

      free(heap->data);
    }

    free(heap);
  }
}

/*
 * Swap 2 items of a heap.
 */
static void __heap_swap(struct heap_t *heap, int i, int j)
{
  void *tmp = heap->data[i];
  heap->data[i] = heap->data[j];
  heap->data[j] = tmp;
}

/*
 * Min heapify at index i.
 */
static void __min_heapify(struct heap_t *heap, int i)
{
  int smallest, left, right;

  if (i < 0)
    return;

  /* get left/right nodes */
  smallest = i;
  left = heap_left(i);
  right = heap_right(i);

  /* get smallest data from parent, left, right */
  if (left < heap->size && heap->compare_func(heap->data[left], heap->data[smallest]) < 0)
    smallest = left;
  if (right < heap->size && heap->compare_func(heap->data[right], heap->data[smallest]) < 0)
    smallest = right;

  /* put smallest at parent index and recursive heapify */
  if (smallest != i) {
    __heap_swap(heap, i, smallest);
    __min_heapify(heap, smallest);
  }
}

/*
 * Max heapify at index i.
 */
static void __max_heapify(struct heap_t *heap, int i)
{
  int largest, left, right;

  if (i < 0)
    return;

  /* get left/right nodes */
  largest = i;
  left = heap_left(i);
  right = heap_right(i);

  /* get largest data from parent, left, right */
  if (left < heap->size && heap->compare_func(heap->data[left], heap->data[largest]) > 0)
    largest = left;
  if (right < heap->size && heap->compare_func(heap->data[right], heap->data[largest]) > 0)
    largest = right;

  /* put largest at parent index and recursive heapify */
  if (largest != i) {
    __heap_swap(heap, i, largest);
    __max_heapify(heap, largest);
  }
}

/*
 * Insert data into a heap.
 */
void heap_insert(struct heap_t *heap, void *data)
{
  int i, cmp;

  /* heap overflow */
  if (heap_is_full(heap))
    return;

  /* add data */
  heap->data[heap->size++] = data;

  /* fix new data position (must be greater than its parent if min heap) */
  for (i = heap->size - 1; i > 0; i = heap_parent(i)) {
    cmp = heap->compare_func(heap->data[heap_parent(i)], heap->data[i]);
    if ((heap->type == HEAP_MIN && cmp < 0) || (heap->type == HEAP_MAX && cmp > 0))
      break;

    __heap_swap(heap, i, heap_parent(i));
  }
}

/*
 * Get minimum value from heap.
 */
void *heap_min(struct heap_t *heap)
{
  void *root;

  if (heap->size <= 0 || heap->type != HEAP_MIN)
    return NULL;

  if (heap->size == 1) {
    heap->size--;
    return heap->data[0];
  }

  /* extract root = minimum item */
  root = heap->data[0];
  heap->data[0] = heap->data[heap->size - 1];
  heap->size--;

  /* reheapify from root */
  __min_heapify(heap, 0);

  return root;
}

/*
 * Get maximum value from heap.
 */
void *heap_max(struct heap_t *heap)
{
  void *root;

  if (heap->size <= 0 || heap->type != HEAP_MAX)
    return NULL;

  if (heap->size == 1) {
    heap->size--;
    return heap->data[0];
  }

  /* extract root = maximum item */
  root = heap->data[0];
  heap->data[0] = heap->data[heap->size - 1];
  heap->size--;

  /* reheapify from root */
  __max_heapify(heap, 0);

  return root;
}
