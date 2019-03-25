#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
volatile sig_atomic_t done = 0;


void intHandler(int sig)
{
    if (sig == SIGINT)
	{
	    done = 1;
    	    printf("User requests termination.\n");
	}
}

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
	long int i = 0;	 	/* loop iterations */
	double pos = 0.0;
	int data_size = 10000;
	double *data;
	double *time;
	double *time_pre;
	double *time_diff;
	
	data = (double *)malloc(data_size * sizeof(double));
	time = (double *)malloc(data_size * sizeof(double));
	time_pre = (double *)malloc(data_size * sizeof(double));
	time_diff = (double *)malloc(data_size * sizeof(double));

	data[0] = time[0] = time_pre[0] = time_diff[0] = 0.0;

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
		
	/* initialise memory data*/
    	ticket_lock(&shared_out->queue);
	shared_out->chk = 0;
	shared_out->act_pos = (double)0.0;
	shared_out->timestamp.sec = 0;
	shared_out->timestamp.nsec = 0;
	ticket_unlock(&shared_out->queue);
	
	/* timebase */
	struct timespec	ts, tleft;
	int ht, toff;
	int64_t cycletime;
	double current_time;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	ht = (ts.tv_nsec / 1000000) + 1; /* round to nearest ms */
	ts.tv_nsec = ht * 1000000;
	cycletime = *(int*)ptr * 1000; /* cycletime in ns */
	toff = 0;

	/* run in shared memory */
	while(!done && i<=data_size) {

		/* timer */
		/* record current time */
		current_time = (double)(ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec) / (NSEC_PER_SEC);
		/* calculate next cycle start */
		add_timespec(&ts, cycletime + toff);
		/* wait to cycle start */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

		/* update master output */		
		/* request ticket lock */
		ticket_lock(&shared_out->queue);
		/* send valid data */
		shared_out->chk = 1;
		/* send current time */
		shared_out->timestamp.sec = ts.tv_sec;
		shared_out->timestamp.nsec = ts.tv_nsec;
		/* send actual position */
		pos = sin((double)(2*M_PI * current_time));
		shared_out->act_pos = pos;
		/* release ticket lock */
		ticket_unlock(&shared_out->queue);
		
		data[i] = pos;
		time_pre[i] = time[i-1];
		time[i] = current_time;
		time_diff[i] = time[i] - time_pre[i];		
		printf("current_time: %lf, pos: %lf, cycle: %ld\r", time[i], data[i], i);

		i++;
	}

	/* write data to a file */
	FILE *fptr = fopen("shm.data", "w");
	for (long int j = 0; j<data_size; j++) {
		if (j>i) time[j] = data[j] = 0;
		fprintf(fptr, "%+lf, %+lf, %+lf, %+lf\n", time[j], time_pre[j], time_diff[j], data[j]);
	}
	
	fclose(fptr);

	free(data);
	free(time);
	free(time_pre);
	free(time_diff);

	return 0;
}

int main(int argc, char *argv[]) 
{
	signal(SIGINT, intHandler);

	int ctime = 1000;
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
		
		/* wait until sub-thread is finished */
		pthread_join(RT_thread, NULL);
	}
	
	return 0;
}

