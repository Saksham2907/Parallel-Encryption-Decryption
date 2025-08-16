#ifndef CHUNK_QUEUE_H
#define CHUNK_QUEUE_H

#include <stddef.h>
#include <stdatomic.h>

typedef struct {
    size_t total_chunks;
    _Atomic size_t next_idx;  // atomic cursor
    size_t chunk_size;
    size_t last_chunk_size;
} chunk_queue_t;

void cq_init(chunk_queue_t* q, size_t total_chunks, size_t chunk_size, size_t last_chunk_size);
int  cq_get_next(chunk_queue_t* q, size_t* idx);

#endif
