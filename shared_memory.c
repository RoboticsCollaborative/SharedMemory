#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>

#include "shared_data.h"


#define	OPEN_FLAG	O_RDWR|O_CREAT|O_TRUNC
#define	MODE_FLAG	0777
#define	SHM_SIZE	4096


/** Acuqire robust mutex
 *
 * @param[in] mutex	=	mutex lock
 * return 0 if owener died, return -1 on error, return 1 on success.
 */
static int mutex_lock(pthread_mutex_t *mutex) {
	
	int err = pthread_mutex_lock(mutex);

	if (err == 0) {
		return 1; /* Acuqire mutex success */
	} else if (err == EOWNERDEAD) {
		pthread_mutex_consistent(mutex);
		return 0; /* Mutex dead */
	} else {
		perror("pthread_mutex_lock");
		return 0;
	}
}

/** Release robust mutex
 *
 * @param[in] mutex	=	mutex lock
 * return -1 on error, return 0 on success.
*/
static int mutex_unlock(pthread_mutex_t *mutex) {
	
	int err = pthread_mutex_unlock(mutex);

	if (!err) return 0; /* Mutex unlock success */

	perror("pthread_mutex_unlock");
	return -1;
}

/** Acquire ticket lock
 *
 * @param[in] ticket	=	ticket lock.
 * return current queue node.
 */
int ticket_lock(ticket_lock_t *ticket) {

	if (!mutex_lock(&ticket->mutex)) { /* Mutex owner died, recover...*/
		fprintf(stderr, "recover mutex\n");
		return -1;
	}

	return 0;
}

/** Release ticket lock
 *
 * @param[in] ticket	=	ticket lock.
 * return -1 on error, return 0 on success.
 */
int ticket_unlock(ticket_lock_t *ticket) {

	if (mutex_unlock(&ticket->mutex)) {
		fprintf(stderr, "mutex_unlock");
		return -1;
	}

	return 0;
}

/** Initialise ticket mutex lock and condition variable.
 * 
 * @param[in] ticket	=	ticket lock. 
 * return 1 on success.
 */
int ticket_init(ticket_lock_t *ticket) {
	/* Initialise mutex */
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
	pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
	pthread_mutex_init(&ticket->mutex, &mattr);
	pthread_mutexattr_destroy(&mattr);

	return 1;
}

/** Open and map a shared memory file.
 *
 * @param[in] shm_name = 	Shared memory file name.
 * @param[in] p	=	Intermediate pointer.
 * return 0 on success.
 */
int openSharedMemory(char *shm_name, void **p) {

	int fd, ret, err = 0; /* error detector*/
	
	/* Create or open a POSIX shared memory object */
	if (!err) {
		fd = shm_open(shm_name, OPEN_FLAG, MODE_FLAG);	/* return 0 on success, -1 on error */
		err = fd < 0;
	}
	
	/* Resize the shared memory file */
	if (!err) {
		ret = ftruncate(fd, SHM_SIZE);	/* return 0 on success, -1 on error */
		err = fd < 0;
	}

	/* Map shared memory to process virtual memory space */
	if (!err) {
		*p = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);	/* return pointer on success, (void*)-1 on error */
		err = (*p == (void *)-1);
	}

	/* Close the file descriptor */
	if (!err) {
		err = close(fd);
	}
	
	return err;
}


