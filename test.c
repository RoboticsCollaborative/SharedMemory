#define _GNU_SOURCE
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include <sched.h>
#include <time.h>
#include <math.h>

#include "shared_data.h"
#include "shared_memory.h"

#define NSEC_PER_SEC 1000000000

pthread_t RT_thread;

/* Add ns to timespec */
void add_timespec(struct timespec *ts, int64_t addtime) {

	int64_t sec, nsec;

	/* Seperate elapsed time in parts */
	nsec = addtime % NSEC_PER_SEC;		/* Elapsed time in whole seconds */
	sec = (addtime - nsec) / NSEC_PER_SEC;	/* Rest of elapsed time in ns */
	ts->tv_sec += sec;
	ts->tv_nsec += nsec;

	/* nsec is always less than i million */
	if ( ts->tv_nsec > NSEC_PER_SEC ) {
		nsec = ts->tv_nsec % NSEC_PER_SEC;
		ts->tv_sec += (ts->tv_nsec - nsec) / NSEC_PER_SEC;
		ts->tv_nsec = nsec;
	}
	
}

int test(void *ptr) {

	/* local variables */
	void *p;	/* intermediate pointer */
	// int err; 	/* error number */
	int i;	 	/* loop iterations */
	double pos;

	/* instanciate input-output data varibles */
	shared_in_t *shared_in;
	shared_out_t *shared_out;

	/* map data structs to shared memory */
	/* open and obtain shared memory pointers for master-input data */
	if (!openSharedMemory(SHARED_IN, &p)) {
		shared_in = (shared_in_t *) p;
	} else {
		fprintf(stderr, "open(shared_in)\n");
		return -1;
	}

	/* initialise ticket lock */
	ticket_init(&shared_in->queue);

	/* open and obtain shared memory pointers for master-output data */
	if (!openSharedMemory(SHARED_OUT, &p)) {
		shared_out = (shared_out_t *) p;
	} else {
		fprintf(stderr, "open(shared_out)\n");
		return -1;
	}

	/* initialise ticket lock */
	ticket_init(&shared_out->queue);

	/* initialise input-output data */
	shared_out->act_pos = (double)0.0;
	shared_in->tg_pos = (double)0.0;

	/* timebase */
	struct timespec	ts, tleft;
	int ht, toff;
	int64_t cycletime;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ht = (ts.tv_nsec / 1000000) + 1; /* round to nearest ms */
	ts.tv_nsec = ht * 1000000;
	cycletime = *(int*)ptr * 1000; /* cycletime in ns */
	toff = 0;

	/* run in shared memory */
	while(1) {

		/* timer */
		/* calculate next cycle start */
		add_timespec(&ts, cycletime + toff);
		/* wait to cycle start */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

		/* update master output */		
		/* request ticket lock */
		ticket_lock(&shared_out->queue);
		/* send actual position */
		pos = sin((double)(2*M_PI/10000 * i));
		shared_out->act_pos = pos;
		printf("receive target points: %lf\n", pos);
		/* release ticket lock */
		ticket_unlock(&shared_out->queue);


		i++;
	}
	
	return 0;


}

int run()
{
 	while(1);
	return 0;
 
}

int main(int argc, char *argv[]) 
{
	int ctime = 10000;
	struct	sched_param	param;
	int	policy	=	SCHED_FIFO;

	printf("ROS Interface Test\n");

	if (argc > 0) {

		/* Create RT thread */	
		pthread_create(&RT_thread, NULL, (void*) &test, (void*) &ctime);

		/* Scheduler */
		memset(&param, 0, sizeof(param));
		param.sched_priority = 40;
		pthread_setschedparam(RT_thread, policy, &param);
		
		/* Core-Iso */
		cpu_set_t CPU3;
		CPU_ZERO(&CPU3);
		CPU_SET(3, &CPU3);
		pthread_setaffinity_np(RT_thread, sizeof(CPU3), &CPU3); 
		
		/* Main thread */
		run();
	}
	
	return 0;
}

