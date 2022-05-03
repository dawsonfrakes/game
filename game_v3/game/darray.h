#ifndef DARRAY_H
#define DARRAY_H

#include <stddef.h>

#define STATIC_INIT_DARRAY(TYPE) {.element_size_in_bytes = sizeof(TYPE)}

typedef struct DynamicArray {
    void *start;
    size_t element_size_in_bytes;
    size_t num_elements;
} DArray;

DArray darray_init(size_t element_size_in_bytes);
void darray_append(DArray *d, const void *element);
void darray_remove(DArray *d, size_t index);
void darray_free(DArray *d);

#ifdef INCLUDE_SRC

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

DArray darray_init(size_t element_size_in_bytes)
{
    DArray result;
    result.start = NULL;
    result.element_size_in_bytes = element_size_in_bytes;
    result.num_elements = 0;
    return result;
}

void darray_append(DArray *d, const void *element)
{
    const size_t current_size = d->num_elements * d->element_size_in_bytes;
    d->start = realloc(d->start, current_size + d->element_size_in_bytes);
    memcpy(((uint8_t *) d->start) + current_size, element, d->element_size_in_bytes);
    ++d->num_elements;
}

void darray_remove(DArray *d, size_t index)
{
    if (d->num_elements == 0 || d->num_elements <= index)
        return;
    const size_t bytes_to_last_elem_start = (d->num_elements - 1) * d->element_size_in_bytes;
    const void *last_elem = ((uint8_t *) d->start) + bytes_to_last_elem_start;
    memcpy(((uint8_t *) d->start)+(index * d->element_size_in_bytes), last_elem, d->element_size_in_bytes);
    d->start = realloc(d->start, bytes_to_last_elem_start);
    --d->num_elements;
}

void darray_free(DArray *d)
{
    free(d->start);
}

#endif /* INCLUDE_SRC */

#endif /* DARRAY_H */
