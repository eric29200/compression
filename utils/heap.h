#include <stdio.h>

#define HEAP_MIN			1
#define HEAP_MAX			2

/**
 * @brief Heap data structure.
 */
struct heap {
	int 		type;					/* heap type = min or max */
	void **		data;					/* data */
	int 		capacity;				/* heap capcity */
	int 		size;					/* heap size */
	int (*compare_func)(const void *, const void *);	/* compare function */
};

#define heap_is_full(heap)		((heap)->size >= (heap)->capacity)
#define heap_parent(i)			((i) == 0 ? -1 : ((((i) + 1) / 2) - 1))
#define heap_left(i)			(2 * (i) + 1)
#define heap_right(i)			(2 * (i) + 2)

/**
 * @brief Create a heap.
 * 
 * @param type 			heap type (min or max)
 * @param capacity 		heap capacity
 * @param compare_func 		compare function
 * 
 * @return new heap
 */
struct heap *heap_create(int type, int capacity, int (*compare_func)(const void *, const void *));

/**
 * @brief Free a heap.
 * 
 * @param heap 			heap
 */
void heap_free(struct heap *heap);

/**
 * @brief Insert data into a heap.
 * 
 * @param heap 		heap
 * @param data 		data
 */
void heap_insert(struct heap *heap, void *data);

/**
 * @brief Get minimum value from heap.
 * 
 * @return minimum value
 */
void *heap_min(struct heap *heap);

/**
 * @brief Get maximum value from heap.
 * 
 * @return maximum value
 */
void *heap_max(struct heap *heap);
