#include <stdbool.h>
#include <threads.h>

typedef struct __node_t {
    void *data;
    struct __node_t *next;
} node_t;

typedef struct __que_t {
    node_t *head, *tail;
    mtx_t head_lock, tail_lock;
} que_t;

void que_init(que_t *q) {
    node_t *tmp = malloc(sizeof(node_t));
    tmp->next = NULL;
    q->head = q->tail = tmp;
    mtx_init(&q->head_lock, mtx_plain);
    mtx_init(&q->tail_lock, mtx_plain);
}

void que_push(que_t *q, void *data) {
    node_t *tmp = malloc(sizeof(node_t));

    tmp->data = data;
    tmp->next = NULL;

    mtx_lock(&q->tail_lock);
    q->tail->next = tmp;
    q->tail = tmp;
    mtx_unlock(&q->tail_lock);
}

bool que_pop(que_t *q, void *data) {
    mtx_lock(&q->head_lock);
    node_t *tmp = q->head;
    node_t *new_head = tmp->next;

    if (new_head == NULL) {
        mtx_unlock(&q->head_lock);
        return false; // queue was empty
    }

    data = new_head->data;
    q->head = new_head;
    mtx_unlock(&q->head_lock);
    free(tmp);
    return true;
}
