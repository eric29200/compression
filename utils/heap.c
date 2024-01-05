#include <stdio.h>
#include <stdlib.h>

#include "heap.h"
#include "../utils/mem.h"

/*
 * Create a heap.
 */

/**
 * @brief Create a heap.
 * 
 * @param type 			heap type (min or max)
 * @param capacity 		heap capacity
 * @param compare_func 		compare function
 * 
 * @return new heap
 */
struct heap *heap_create(int type, int capacity, int (*compare_func)(const void *, const void *))
{
	struct heap *heap;

	/* check type */
	if (type != HEAP_MIN && type != HEAP_MAX)
		return NULL;

	heap = (struct heap *) xmalloc(sizeof(struct heap));
	heap->type = type;
	heap->capacity = capacity;
	heap->size = 0;
	heap->compare_func = compare_func;
	heap->data = (void **) xmalloc(sizeof(void *) * capacity);

	return heap;
}

/**
 * @brief Free a heap.
 * 
 * @param heap 			heap
 */
void heap_free(struct heap *heap)
{
	if (heap) {
		xfree(heap->data);
		free(heap);
	}
}

/**
 * @brief Swap 2 items in a heap.
 * 
 * @param heap 		heap
 * @param i 		first item index
 * @param j 		second item index
 */
static void __heap_swap(struct heap *heap, int i, int j)
{
	void *tmp = heap->data[i];
	heap->data[i] = heap->data[j];
	heap->data[j] = tmp;
}

/**
 * @brief Min heapify at index i.
 * 
 * @param heap		heap
 * @param i		index
 */
static void __min_heapify(struct heap *heap, int i)
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

/**
 * @brief Max heapify at index i.
 * 
 * @param heap		heap
 * @param i		index
 */
static void __max_heapify(struct heap *heap, int i)
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

/**
 * @brief Insert data into a heap.
 * 
 * @param heap 		heap
 * @param data 		data
 */
void heap_insert(struct heap *heap, void *data)
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

/**
 * @brief Get minimum value from heap.
 * 
 * @return minimum value
 */
void *heap_min(struct heap *heap)
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

/**
 * @brief Get maximum value from heap.
 * 
 * @return maximum value
 */
void *heap_max(struct heap *heap)
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
