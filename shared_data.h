#ifndef SHARED_DATA_H
#define SHARED_DATA_H


#include <stdint.h>
#include <pthread.h>


typedef struct ticket_lock {
    pthread_mutex_t mutex;
} ticket_lock_t;

typedef struct timestamp {
    int64_t sec;
    int64_t nsec;
} timestamp_t;

typedef struct shared_in {
    int chk;
    double tg_pos;
    timestamp_t timestamp;
    ticket_lock_t queue;
} shared_in_t;

typedef struct shared_out {
    int chk;
    double act_pos;
    timestamp_t timestamp;
    ticket_lock_t queue;
} shared_out_t;


#endif /* SHARED_DATA_H */
