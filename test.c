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

#include "shm_data.h"
#include "shm.h"


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
	//shared_in_t *shared_in;
	//shared_out_t *shared_out;
	Rdda *rdda;

	rdda = initRdda();
	for (int i=0; i<2; i++) {
		rdda->motor[i].motorIn.act_pos = 0.0;
		rdda->motor[i].motorIn.act_vel = 0.0;
		rdda->motor[i].rosOut.pos_ref = 0.0;
		rdda->motor[i].rosOut.stiffness = 0.0;
	}

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
	//while(!done && i<=data_size) {
	while(!done) {
		/* timer */
		/* record current time */
		current_time = (double)(ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec) / (NSEC_PER_SEC);
		/* calculate next cycle start */
		add_timespec(&ts, cycletime + toff);
		/* wait to cycle start */
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, &tleft);

	
//		data[i] = pos;
//		time_pre[i] = time[i-1];
//		time[i] = current_time;
//		time_diff[i] = time[i] - time_pre[i];		
//		printf("current_time: %lf, pos: %lf, cycle: %ld\r", time[i], data[i], i);

		mutex_lock(&rdda->mutex);
		printf("pos_ref[0]: %lf, stiffness[0]: %lf\r", rdda->motor[0].rosOut.pos_ref, rdda->motor[0].rosOut.stiffness);
		mutex_unlock(&rdda->mutex);

		//i++;
	}

	/* write data to a file */
//	FILE *fptr = fopen("shm.data", "w");
//	for (long int j = 0; j<data_size; j++) {
//		if (j>i) time[j] = data[j] = 0;
//		fprintf(fptr, "%+lf, %+lf, %+lf, %+lf\n", time[j], time_pre[j], time_diff[j], data[j]);
//	}
	
//	fclose(fptr);

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

