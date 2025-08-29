/*
 * -- DYNAMIC ARRAYS --
 *
 * This file contains helper macro functions to handle dynamic arrays.
 *
 * This library uses struct field names to interact with the struct
 * and so assumes the following structure in order to work properly:
 *
 * typedef struct {
 *     int    *items;
 *     size_t  count;
 *     size_t  capacity;
 * } Numbers;
 *
 * , where:
 *
 * - items is the base array that contains your items
 * - count is the current size of the dynamic array
 * - capacity is the current maximum size of the array
 *
 * -- HOW IT WORKS --
 *
 * All this functions do is operate on arrays, reallocating it in case
 * it has run out of memory for further operations. When it does, it
 * gets resized, scaled by DA_SCALE_FACTOR. Its default value is 2, but
 * you can set it to whatever value you prefer, defining it before including.
 * (Note: A scale factor of 1 or less is pointless)
 *
 * Since this library aims to be generic enough to be used with any struct
 * with matching field names, it offers no way to initialize dynamic arrays.
 * The suggested way to initialize a dynamic array is to zero-initialize the
 * struct. When trying to interact with it, it will be detected that it has
 * zero capacity, so instead of using DA_SCALE_FACTOR to resize, DA_INIT_CAP
 * will be used as default initial capacity. You can also set it.
 *
 * -- REDEFINABLE MACROS --
 *  DA_INIT_CAP                     - Initial capacity
 *  DA_REALLOC(ptr, sz_old, sz_new) - Function for reallocating memory
 *  DA_FREE(ptr, sz)                - Function for freeing allocated memory
 *  DA_MEMCPY(dest, src, sz)        - Function for copying memory
 *  DA_ASSERT(cond)                 - Function for asserting conditions
 *  DA_SORT_ALG(ptr, n, sz, cmp_f)  - Sorting algorithm
 */

#ifndef DA_H_
#define DA_H_

// CONSTANTS

// #ifndef DA_SCALE_FACTOR
// #define DA_SCALE_FACTOR 2
// #endif // DA_SCALE_FACTOR

#ifndef DA_INIT_CAP
#define DA_INIT_CAP 256
#endif // DA_INIT_CAP

#ifndef DA_REALLOC
#include <stdlib.h>
#define DA_REALLOC(ptr, sz_old, sz_new) realloc(ptr, sz_new)
#endif // DA_REALLOC

#ifndef DA_FREE
#include <stdlib.h>
#define DA_FREE(ptr, sz) free(ptr)
#endif // DA_FREE

#ifndef DA_MEMCPY
#include <string.h>
#define DA_MEMCPY(dest, src, sz) memcpy(dest, src, sz)
#endif // DA_MEMCPY

#ifndef DA_ASSERT
#include <assert.h>
#define DA_ASSERT(cond) assert(cond)
#endif // DA_ASSERT

#ifndef DA_SORT_ALG
#include <stdlib.h>
#define DA_SORT_ALG(ptr, item_count, item_size, compare_func) qsort(ptr, item_count, item_size, compare_func)
#endif // DA_SORT_ALG

// MEMORY MANAGEMENT

// TODO Let user define scale factor
#define da_reserve(da, expected_cap)                                                               \
do {                                                                                               \
    if ((expected_cap) > (da)->capacity) {                                                         \
        unsigned int cap_old = (da)->capacity;                                                     \
                                                                                                   \
        if ((da)->capacity == 0) {                                                                 \
            (da)->capacity = DA_INIT_CAP;                                                          \
        }                                                                                          \
                                                                                                   \
        while ((da)->capacity < expected_cap) {                                                    \
            (da)->capacity <<= 1;                                                                  \
        }                                                                                          \
                                                                                                   \
        (da)->items = DA_REALLOC((da)->items, cap_old, (da)->capacity * sizeof(*(da)->items));     \
    }                                                                                              \
} while (0)

#define da_free(da)                                                                                \
do {                                                                                               \
    DA_FREE((da)->items);                                                                          \
} while (0)

// CORE OPERATIONS

#define da_append(da, item)                                                                        \
do {                                                                                               \
    da_reserve(da, (da)->count + 1);                                                               \
    (da)->items[(da)->count++] = (item);                                                           \
} while (0)

#define da_append_many(da, new_items, new_items_count)                                             \
do {                                                                                               \
    da_reserve(da, (da)->count + (new_items_count));                                               \
    DA_MEMCPY((da)->items + (da)->count, (new_items), (new_items_count) * sizeof(*(da)->items));   \
    (da)->count += (new_items_count);                                                              \
} while (0)

#define da_pop(da) (DA_ASSERT((da)->count > 0), (da)->items[(da)->count--])

#define da_remove_unordered(da, i)                                                                 \
do {                                                                                               \
    unsigned int j = (i);                                                                          \
    DA_ASSERT((da)->count > j);                                                                    \
    (da)->items[j] = (da)->items[--(da)->count];                                                   \
} while (0)

// HIGH ORDER FUNCTIONS
// SORT AND SEARCH

#define da_foreach(type, it, da) for (type *it = 0; it < (da)->items + (da)->count; ++it)

#define da_sort(da, cmp_f) (DA_SORT_ALG((da)->items, (da)->count, sizeof(*(da)->items), cmp_f))

#endif // DA_H_
