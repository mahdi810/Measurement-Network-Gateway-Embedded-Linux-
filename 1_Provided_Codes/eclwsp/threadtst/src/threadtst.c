/*
 ============================================================================
 Name        : threadtst.c
 Author      : Kai Mueller
 Version     : 0.0a
 Copyright   : K. Mueller. 20-DEC-2021
 Description : Threads example
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

#define OBUF_SIZE 512
static char Output_Buf[OBUF_SIZE];

typedef struct {
	pthread_t thobj;
	pthread_mutex_t thmutex;
	pthread_cond_t thcond;
	int  thkill;
} PThread_struct;


static PThread_struct  StdOut_Thread;

static PThread_struct  Periodic_Thread;


static double GetElapsedTime()
{
	double etime;
	uint64_t us_current_time;

	clock_gettime(CLOCK_MONOTONIC, &Current_Time);
	us_current_time = Current_Time.tv_sec * 1000000 + Current_Time.tv_nsec / 1000;
	etime = 1.0e-6 * (us_current_time - US_Startup_Time);
	return  etime;
}


void *CyclicThread (void *arg)
{
	int cycle_count = 0;
	int tim_fd;
	struct itimerspec itval;
	unsigned long long missed, wakeups_missed;
	double etime;


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

	while (Periodic_Thread.thkill == 0)
	{
		etime = GetElapsedTime();
		printf("[Cyclic Thread]: %d (%llu) time: %f\n",
				cycle_count, wakeups_missed, etime);
		cycle_count++;

		read (tim_fd, &missed, sizeof (missed));
		if (missed > 0) {
			wakeups_missed += missed - 1;
		}
	}
	printf("exiting CyclicThread.\n");
	pthread_exit(NULL);
}



static void *OutputThread(void *ot_ptr)
{
	for (;;) {
		pthread_mutex_lock(&StdOut_Thread.thmutex);
		pthread_cond_wait(&StdOut_Thread.thcond, &StdOut_Thread.thmutex);
		printf("[OT]%s\n", Output_Buf);
		if (StdOut_Thread.thkill != 0)  break;
		pthread_mutex_unlock(&StdOut_Thread.thmutex);
	}
	printf("exiting OutputThread.\n");
	pthread_exit(NULL);
}

static void ThrPrint(char *str)
{
	pthread_mutex_lock(&StdOut_Thread.thmutex);
	strncpy(Output_Buf, str, OBUF_SIZE);
	pthread_cond_signal(&StdOut_Thread.thcond);
	pthread_mutex_unlock(&StdOut_Thread.thmutex);
}


#define BUF_SIZE 128

int main(int argc , char *argv[])
{
	int  rc, terminate;
	double  ctime;
	char cbuf[BUF_SIZE];

	printf("--- ThreadTest V0.0a ---\n");
	Startup_Time.tv_sec = 0;
	Startup_Time.tv_nsec = 0;
	if (clock_gettime(CLOCK_MONOTONIC, &Startup_Time) == 0) {
		printf("Start_Time received.\n");
	} else {
		printf("Error getting CLOCK_MONOTIC time.\n");
	}
	US_Startup_Time = Startup_Time.tv_sec * 1000000 + Startup_Time.tv_nsec / 1000;
	ctime = GetElapsedTime();
	printf("Elapsed time: %f\n", ctime);

	Output_Buf[0] = '\0';

	// thread for stdout
	rc = pthread_mutex_init(&StdOut_Thread.thmutex, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from output mutex_init() is %d\n", rc);
	   exit(-1);
	}
	// pthread_mutex_lock(&StdOut_Thread.thmutex);

	StdOut_Thread.thkill = 0;
	rc = pthread_create(&StdOut_Thread.thobj, NULL, OutputThread, NULL);
	if (rc != 0) {
		printf("ERROR; return code from output pthread_create() is %d\n", rc);
		exit(-1);
	 }

	Periodic_Thread.thkill = 0;
	rc = pthread_create(&Periodic_Thread.thobj, NULL, CyclicThread, NULL);
	if (rc != 0) {
		printf("ERROR; return code from cyclic pthread_create() is %d\n", rc);
		exit(-1);
	 }

	terminate = 0;
	do {
		printf(">> ");  fflush(stdout);
		fgets(cbuf, BUF_SIZE, stdin);
		cbuf[BUF_SIZE-1] = '\0';
		if (strncmp(cbuf, "exit", 4) == 0) {
			StdOut_Thread.thkill = 1;
			pthread_mutex_lock(&StdOut_Thread.thmutex);
			pthread_cond_signal(&StdOut_Thread.thcond);
			pthread_mutex_unlock(&StdOut_Thread.thmutex);
			Periodic_Thread.thkill = 1;
			terminate = 1;
		} else if (strncmp(cbuf, "thprint", 7) == 0) {
			ThrPrint("---Test---");
		} else {
			printf(" *** unknown command\n");
		}
	} while (!terminate);

	printf(" o waiting for thread joins...\n");
	rc = pthread_join(StdOut_Thread.thobj, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from output pthread_join() is %d\n", rc);
	   exit(-1);
	}
	pthread_mutex_destroy(&StdOut_Thread.thmutex);

	rc = pthread_join(Periodic_Thread.thobj, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from cyclic pthread_join() is %d\n", rc);
	   exit(-1);
	}

	printf("ThreadTest shutdown complete.\n");
	return EXIT_SUCCESS;
}
