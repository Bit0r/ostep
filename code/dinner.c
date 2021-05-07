#include <stdbool.h>
#include <stdio.h>
#include <threads.h>
#include <time.h>

mtx_t forks[5];

void think() { thrd_sleep(&(struct timespec){.tv_nsec = 100000000}, NULL); }

void get_forks(int id) {
    if (id == 4) {
        mtx_lock(&forks[0]);
        mtx_lock(&forks[id]);
    } else {
        mtx_lock(&forks[id]);
        mtx_lock(&forks[id + 1]);
    }
}

void eat(int id) { printf("哲学家%d正在吃饭\n", id); }

void put_forks(int id) {
    mtx_unlock(&forks[id]);
    mtx_unlock(&forks[(id + 1) % 5]);
}

int ph(void *id) {
    while (true) {
        think();
        get_forks((int)id);
        eat((int)id);
        put_forks((int)id);
    }
    return 0;
}

int main() {
    thrd_t thrs[5];
    for (int i = 0; i < 5; i++) {
        mtx_init(&forks[i], mtx_plain);
    }
    for (int i = 0; i < 5; i++) {
        thrd_create(&thrs[i], ph, (void *)i);
    }
    for (int i = 0; i < 5; i++) {
        thrd_join(&thrs[i], NULL);
    }
    return 0;
}
