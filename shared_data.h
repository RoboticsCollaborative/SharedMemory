#ifndef SHARED_DATA_H
#define SHARED_DATA_H


#include <stdint.h>
#include <pthread.h>


typedef struct ticket_lock {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int queue_head, queue_tail;
} ticket_lock_t;

typedef struct shared_in {
    double tg_pos;
    ticket_lock_t queue;
} shared_in_t;

typedef struct shared_out {
    double act_pos;
    ticket_lock_t queue;
} shared_out_t;


#endif /* SHARED_DATA_H */
