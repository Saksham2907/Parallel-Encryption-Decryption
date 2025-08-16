#include "chunk_queue.h"

void cq_init(chunk_queue_t* q, size_t total_chunks, size_t chunk_size, size_t last_chunk_size) {
    q->total_chunks = total_chunks;
    q->chunk_size = chunk_size;
    q->last_chunk_size = last_chunk_size;
    atomic_store(&q->next_idx, 0);
}

int cq_get_next(chunk_queue_t* q, size_t* idx) {
    size_t i = atomic_fetch_add(&q->next_idx, 1);
    if (i >= q->total_chunks) return 0;
    *idx = i;
    return 1;
}
