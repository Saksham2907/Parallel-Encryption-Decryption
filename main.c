#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#include "chunk_queue.h"

#define CHUNK_SIZE (1<<20)  // 1MB/chunk
#define KEY 0x5Au           // XOR demo key (replace with AES/OpenSSL if needed)

typedef struct {
    uint8_t* in;
    uint8_t* out;
    chunk_queue_t* q;
    size_t total_size;
    int decrypt;            // 0=encrypt, 1=decrypt (same for XOR)
} worker_ctx_t;

static void* worker(void* arg) {
    worker_ctx_t* ctx = (worker_ctx_t*)arg;
    size_t idx;
    while (cq_get_next(ctx->q, &idx)) {
        size_t off = idx * CHUNK_SIZE;
        size_t sz = (idx == ctx->q->total_chunks - 1) ? ctx->q->last_chunk_size : CHUNK_SIZE;
        for (size_t i = 0; i < sz; ++i) {
            ctx->out[off+i] = ctx->in[off+i] ^ KEY; // XOR encrypt/decrypt
        }
    }
    return NULL;
}

static void die(const char* msg) {
    perror(msg);
    exit(1);
}

static void run_stage(const char* in_path, const char* out_path, int decrypt) {
    // read input
    int fd = open(in_path, O_RDONLY);
    if (fd < 0) die("open input");
    struct stat st;
    if (fstat(fd, &st) < 0) die("fstat");
    size_t total = (size_t)st.st_size;
    uint8_t* ibuf = mmap(NULL, total, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ibuf == MAP_FAILED && total>0) die("mmap input");
    close(fd);

    // output file
    int ofd = open(out_path, O_CREAT|O_RDWR|O_TRUNC, 0644);
    if (ofd < 0) die("open output");
    if (ftruncate(ofd, (off_t)total) < 0) die("ftruncate");
    uint8_t* obuf = mmap(NULL, total, PROT_READ|PROT_WRITE, MAP_SHARED, ofd, 0);
    if (obuf == MAP_FAILED && total>0) die("mmap output");
    close(ofd);

    // queue
    size_t chunks = (total + CHUNK_SIZE - 1)/CHUNK_SIZE;
    size_t last = (chunks==0)?0:(total - (chunks-1)*CHUNK_SIZE);
    chunk_queue_t q;
    cq_init(&q, chunks, CHUNK_SIZE, last);

    // threads
    int nthreads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (nthreads <= 0) nthreads = 4;
    pthread_t* th = malloc(sizeof(pthread_t)*nthreads);
    worker_ctx_t ctx = { .in=ibuf, .out=obuf, .q=&q, .total_size=total, .decrypt=decrypt };

    for (int i=0;i<nthreads;i++)
        pthread_create(&th[i], NULL, worker, &ctx);
    for (int i=0;i<nthreads;i++)
        pthread_join(th[i], NULL);

    if (total>0) {
        msync(obuf, total, MS_SYNC);
        munmap(ibuf, total);
        munmap(obuf, total);
    }
    free(th);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage:\n  %s <in> <out.enc>\n  %s -d <in.enc> <out>\n", argv[0], argv[0]);
        return 1;
    }

    int decrypt = 0;
    int argi = 1;
    if (strcmp(argv[1], "-d")==0) { decrypt=1; argi=2; }
    if (argc-argi < 2) {
        fprintf(stderr, "Invalid args\n");
        return 1;
    }

    const char* in_path  = argv[argi];
    const char* out_path = argv[argi+1];

    // Two-process pipeline: P0 does stage; (for demo, just one stage)
    // If you want encrypt->sharedmem->decrypt verify in one go, fork here.
    pid_t pid = fork();
    if (pid < 0) die("fork");
    if (pid == 0) {
        // child: do work
        run_stage(in_path, out_path, decrypt);
        _exit(0);
    } else {
        int status=0;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
            fprintf(stderr, "child failed\n");
            return 1;
        }
        fprintf(stdout, "Done: %s -> %s\n", in_path, out_path);
    }
    return 0;
}
