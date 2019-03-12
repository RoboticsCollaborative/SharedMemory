#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <math.h>

#include "shared_data.h"
#include "shared_memory.h"


int main () {

	/* Local variables */
	void *p;	/* Intermediate pointer */
	// int err; 	/* Error number */
	int i;	 	/* Loop iterations */
	double pos = 0.0; 

	/* Instanciate input-output data varibles */
	shared_in_t *shared_in;
	shared_out_t *shared_out;

	/* Map data structs to shared memory */
	/* Open and obtain shared memory pointers for master-input data */
	if (!openSharedMemory(SHARED_IN, &p)) {
		shared_in = (shared_in_t *) p;
	} else {
		fprintf(stderr, "Open(shared_in)\n");
		return -1;
	}

	/* Initialise ticket lock */
	ticket_init(&shared_in->queue);

	/* Open and obtain shared memory pointers for master-output data */
	if (!openSharedMemory(SHARED_OUT, &p)) {
		shared_out = (shared_out_t *) p;
	} else {
		fprintf(stderr, "Open(shared_out)\n");
		return -1;
	}

	/* Initialise ticket lock */
	ticket_init(&shared_out->queue);

	/* Run in shared memory */
	while(1) {

		/* Update master input */		
		/* Request ticket lock */
		ticket_lock(&shared_in->queue);
		/* Generate waypoints */
		pos = sin((double)(2*M_PI/1000 * i));
		/* Send target position */
		shared_in->tg_pos = pos;
		/* Release ticket lock */
		ticket_unlock(&shared_in->queue);

		/* Update master output */
		/* Request ticket lock */
		ticket_lock(&shared_out->queue);
		/* Receive actual position */
		pos = shared_out->act_pos;
		printf("Receive actual points: %lf\n", pos);
		/* Release ticket lock */
		ticket_unlock(&shared_out->queue);
		
		i++;
		usleep(20000);
	}

}
