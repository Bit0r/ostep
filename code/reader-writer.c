#include <stdio.h>
#include <threads.h>

typedef struct {
    mtx_t rw_mutex, mutex;
    size_t readers;
} rw_mtx_t;

void rw_mtx_init(rw_mtx_t *rw) {
    rw->readers = 0;
    mtx_init(&rw->rw_mutex, mtx_plain);
    mtx_init(&rw->mutex, mtx_plain);
}

void rw_mtx_readlock(rw_mtx_t *rw) {
    mtx_lock(&rw->mutex);
    if (rw->readers == 0) {
        mtx_lock(&rw->rw_mutex);
    }
    rw->readers++;
    mtx_unlock(&rw->mutex);
}

void rw_mtx_readunlock(rw_mtx_t *rw) {
    mtx_lock(&rw->mutex);
    rw->readers--;
    if (rw->readers == 0) {
        mtx_unlock(&rw->rw_mutex);
    }
    mtx_unlock(&rw->mutex);
}

void rw_mtx_writelock(rw_mtx_t *rw) { mtx_lock(&rw->rw_mutex); }

void rw_mtx_writeunlock(rw_mtx_t *rw) { mtx_unlock(&rw->rw_mutex); }
