#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <math.h>

#include "./shared_data.h"

//#define SHARED_IN "/shared_input"
//#define SHARED_OUT "/shared_output"

char* SHARED_IN = (char*)"/shared_input";
char* SHARED_OUT = (char*)"/shared_output";

int openSharedMemory(char *shm_name, void **p);
int ticket_init(ticket_lock_t *ticket);
int ticket_lock(ticket_lock_t *ticket); 
int ticket_unlock(ticket_lock_t *ticket); 

#endif /* SHARED_MEMORY_H */
