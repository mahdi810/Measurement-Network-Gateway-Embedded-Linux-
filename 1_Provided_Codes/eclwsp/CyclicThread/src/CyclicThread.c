/*
 ============================================================================
 Name        : CyclicThread.c
 Author      : Kai Mueller
 Version     : 0.0a
 Copyright   : K. Mueller. 12-NOV-2022
 Description : hreads example
 ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <time.h>


static struct timespec Startup_Time, Current_Time;
static uint64_t US_Startup_Time;

static volatile int Thread_Exec;

typedef struct {
	pthread_t thobj;
	pthread_mutex_t thmutex;
	pthread_cond_t thcond;
	short int  thkill;
} PThread_struct;


static PThread_struct  Thread_0, Thread_1;

static PThread_struct  Thread_Periodic;


static uint64_t GetElapsedTime()
{
	uint64_t us_current_time, etime;

	clock_gettime(CLOCK_MONOTONIC, &Current_Time);
	us_current_time = Current_Time.tv_sec * 1000000 + Current_Time.tv_nsec / 1000;
	etime = us_current_time - US_Startup_Time;
	return  etime;
}

static void *ThTask_0(void *arg)
{
	uint64_t etime;

	while (Thread_0.thkill == 0) {
		pthread_mutex_lock(&Thread_0.thmutex);
		pthread_cond_wait(&Thread_0.thcond, &Thread_0.thmutex);
		etime = GetElapsedTime();
		printf("--Thread_0 time: %lu\n", etime);
		pthread_mutex_unlock(&Thread_0.thmutex);
	}
	printf("exiting Thread_0.\n");
	pthread_exit(NULL);
}


static void *ThTask_1(void *arg)
{
	uint64_t etime;
	while (Thread_1.thkill == 0) {
		pthread_mutex_lock(&Thread_1.thmutex);
		pthread_cond_wait(&Thread_1.thcond, &Thread_1.thmutex);
		etime = GetElapsedTime();
		printf("--Thread_1 time: %lu\n", etime);
		pthread_mutex_unlock(&Thread_1.thmutex);
	}
	printf("exiting Thread_1.\n");
	pthread_exit(NULL);
}



void *CyclicThread (void *arg)
{
	int cycle_count = 0;
	int tim_fd;
	struct itimerspec itval;
	unsigned long long missed, wakeups_missed;
	uint64_t etime;


	wakeups_missed = 0;
	/* Create the timer */
	tim_fd = timerfd_create (CLOCK_MONOTONIC, 0);
	if (tim_fd == -1) {
		printf(" *** error creating timer.");
	}
	/* Make the timer periodic */
	itval.it_interval.tv_sec = 1;
	itval.it_interval.tv_nsec = 0;
	itval.it_value.tv_sec = 1;
	itval.it_value.tv_nsec = 0;
	timerfd_settime (tim_fd, 0, &itval, NULL);

	while (Thread_Periodic.thkill == 0)
	{
		etime = GetElapsedTime();
		printf("[Cyclic Thread]: %d (%llu) time: %lu\n",
				cycle_count, wakeups_missed, etime);
		cycle_count++;

		read(tim_fd, &missed, sizeof(missed));
		if (missed > 0) {
			wakeups_missed += missed - 1;
		}
		if (Thread_Exec == 0) {
			pthread_mutex_lock(&Thread_0.thmutex);
			pthread_cond_signal(&Thread_0.thcond);
			pthread_mutex_unlock(&Thread_0.thmutex);
			Thread_Exec = 1;
		} else {
			pthread_mutex_lock(&Thread_1.thmutex);
			pthread_cond_signal(&Thread_1.thcond);
			pthread_mutex_unlock(&Thread_1.thmutex);
			Thread_Exec = 0;
		}
	}
	printf("exiting CyclicThread.\n");
	pthread_exit(NULL);
}



#define BUF_SIZE 128

int main(int argc , char *argv[])
{
	int  rc, terminate;
	uint64_t  etime;
	char cbuf[BUF_SIZE];

	printf("--- Cyclic Thread Test V0.0a ---\n");
	Thread_Exec = 0;
	Startup_Time.tv_sec = 0;
	Startup_Time.tv_nsec = 0;
	if (clock_gettime(CLOCK_MONOTONIC, &Startup_Time) == 0) {
		printf("Start_Time received.\n");
	} else {
		printf("Error getting CLOCK_MONOTIC time.\n");
	}
	US_Startup_Time = Startup_Time.tv_sec * 1000000 + Startup_Time.tv_nsec / 1000;
	etime = GetElapsedTime();
	printf("Elapsed time: %lu\n", etime);

	// thread #0
	rc = pthread_mutex_init(&Thread_0.thmutex, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from Task_0 mutex_init() is %d\n", rc);
	   exit(-1);
	}
	Thread_0.thkill = 0;
	rc = pthread_create(&Thread_0.thobj, NULL, ThTask_0, NULL);
	if (rc != 0) {
		printf("ERROR; return code from Task_0 pthread_create() is %d\n", rc);
		exit(-1);
	 }

	// thread #1
	rc = pthread_mutex_init(&Thread_1.thmutex, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from Task_1 mutex_init() is %d\n", rc);
	   exit(-1);
	}
	Thread_0.thkill = 0;
	rc = pthread_create(&Thread_1.thobj, NULL, ThTask_1, NULL);
	if (rc != 0) {
		printf("ERROR; return code from Task_1 pthread_create() is %d\n", rc);
		exit(-1);
	 }

	Thread_Periodic.thkill = 0;
	rc = pthread_create(&Thread_Periodic.thobj, NULL, CyclicThread, NULL);
	if (rc != 0) {
		printf("ERROR; return code from cyclic pthread_create() is %d\n", rc);
		exit(-1);
	 }

	terminate = 0;
	while (terminate == 0) {
		printf(">> ");  fflush(stdout);
		fgets(cbuf, BUF_SIZE, stdin);
		cbuf[BUF_SIZE-1] = '\0';
		if (strncmp(cbuf, "exit", 4) == 0) {
			Thread_0.thkill = 1;
			pthread_mutex_lock(&Thread_0.thmutex);
			pthread_cond_signal(&Thread_0.thcond);
			pthread_mutex_unlock(&Thread_0.thmutex);
			Thread_Periodic.thkill = 1;
			Thread_1.thkill = 1;
			pthread_mutex_lock(&Thread_1.thmutex);
			pthread_cond_signal(&Thread_1.thcond);
			pthread_mutex_unlock(&Thread_1.thmutex);
			terminate = 1;
		} else {
			printf(" *** unknown command\n");
		}
	};

	printf(" o waiting for thread joins...\n");
	rc = pthread_join(Thread_0.thobj, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from output pthread_join() is %d\n", rc);
	   exit(-1);
	}
	pthread_mutex_destroy(&Thread_0.thmutex);

	rc = pthread_join(Thread_1.thobj, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from output pthread_join() is %d\n", rc);
	   exit(-1);
	}
	pthread_mutex_destroy(&Thread_1.thmutex);

	Thread_Periodic.thkill = 1;
	rc = pthread_join(Thread_Periodic.thobj, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from cyclic pthread_join() is %d\n", rc);
	   exit(-1);
	}

	printf("Cyclic Thread Test shutdown complete.\n");
	return EXIT_SUCCESS;
}
