/*
 ============================================================================
 Name        : mthread.c
 Author      : Kai Mueller
 Version     : 0.0a
 Copyright   : K. Mueller. 07-DEC-2023
 Description : measurement thread
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


typedef struct {
	pthread_t thobj;
	pthread_mutex_t thmutex;
	pthread_cond_t thcond;
	short int  thkill;
} PThread_struct;


static PThread_struct  Thread_Periodic;

#define RBUF_LEN  10

typedef struct {
	pthread_mutex_t bmutex;
	volatile int sensor_id[RBUF_LEN];
	volatile int rb_pointer, rb_cnt;
	volatile uint64_t tstamp[RBUF_LEN];
	volatile int svalue[RBUF_LEN];
} RingB_struct;

static RingB_struct Ring_Buf;
static short int Do_Sample;

static int Sensor_Value;

static void PushRB(int sid, uint64_t ts, int sv)
{
	Ring_Buf.sensor_id[Ring_Buf.rb_pointer] = sid;
	Ring_Buf.tstamp[Ring_Buf.rb_pointer] = ts;
	Ring_Buf.svalue[Ring_Buf.rb_pointer] = sv;
	if (Ring_Buf.rb_cnt < RBUF_LEN) {
		Ring_Buf.rb_cnt++;
	}
	Ring_Buf.rb_pointer = (Ring_Buf.rb_pointer + 1) % RBUF_LEN;
}

static int PopRB(int *sid, uint64_t *ts, int *sv)
{
	int  rc, k;

	rc = 0;
	if (Ring_Buf.rb_cnt > 0) {
		rc = 1;
		k = Ring_Buf.rb_pointer - Ring_Buf.rb_cnt;
		if (k < 0) {
			k += RBUF_LEN;
		}
		*sid = Ring_Buf.sensor_id[k];
		*ts =  Ring_Buf.tstamp[k];
		*sv =  Ring_Buf.svalue[k];
		Ring_Buf.rb_cnt--;
	}
	return rc;
}

static uint64_t GetElapsedTime()
{
	uint64_t us_current_time, etime;

	clock_gettime(CLOCK_MONOTONIC, &Current_Time);
	us_current_time = Current_Time.tv_sec * 1000000 + Current_Time.tv_nsec / 1000;
	etime = us_current_time - US_Startup_Time;
	return  etime;
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
	itval.it_interval.tv_sec = 0;
	itval.it_interval.tv_nsec = 500000000;
	itval.it_value.tv_sec = 0;
	itval.it_value.tv_nsec = 500000000;
	timerfd_settime (tim_fd, 0, &itval, NULL);

	while (Thread_Periodic.thkill == 0)
	{
		etime = GetElapsedTime();
		if (Do_Sample > 0) {
			pthread_mutex_lock(&Ring_Buf.bmutex);
			PushRB(42, etime, Sensor_Value);
			pthread_mutex_unlock(&Ring_Buf.bmutex);
			Sensor_Value += 2;
		}
		// printf("[Cyclic Thread]: %d (%llu) time: %lu\n", cycle_count, wakeups_missed, etime);
		cycle_count++;

		read(tim_fd, &missed, sizeof(missed));
		if (missed > 0) {
			wakeups_missed += missed - 1;
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
	int sid, sv, mcnt;
	uint64_t ts;

	printf("--- Measurement Thread V0.1a ---\n");
	Startup_Time.tv_sec = 0;
	Startup_Time.tv_nsec = 0;
	if (clock_gettime(CLOCK_MONOTONIC, &Startup_Time) == 0) {
		printf("Start_Time received.\n");
	} else {
		printf("Error getting CLOCK_MONOTIC time.\n");
	}
	US_Startup_Time = Startup_Time.tv_sec * 1000000 + Startup_Time.tv_nsec / 1000;
	etime = GetElapsedTime();
	printf("Elapsed time: %llu us.\n", etime);

	Sensor_Value = 100;
	Do_Sample = 0;
	//initialize ring buffer
	rc = pthread_mutex_init(&Ring_Buf.bmutex, NULL);
	if (rc != 0) {
	   printf("ERROR: return code for Ring_Buf.bmutex init is %d\n", rc);
	   exit(-1);
	}
	Ring_Buf.rb_cnt = 0;
	Ring_Buf.rb_pointer = 0;

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
			terminate = 1;
		} else if (strncmp(cbuf, "start", 5) == 0) {
			Do_Sample = 1;
			printf("start measurements...\n");
		} else if (strncmp(cbuf, "stop", 4) == 0) {
			Do_Sample = 0;
			printf("measurements stopped.\n");
		} else if (strncmp(cbuf, "getm", 4) == 0) {
			mcnt = 0;
			pthread_mutex_lock(&Ring_Buf.bmutex);
			while (PopRB(&sid, &ts, &sv) > 0) {
				mcnt++;
				printf("%4d. sid: %4d  ts: %llu  sv: %6d\n", mcnt, sid, ts, sv);
			}
			pthread_mutex_unlock(&Ring_Buf.bmutex);
			if (mcnt == 0) {
				printf("no sensor data available.\n");
			} else {
				printf("...no more data.\n");
			}
		} else {
			printf(" *** unknown command\n");
		}
	};

	printf(" o waiting for thread joins...\n");
	Thread_Periodic.thkill = 1;
	rc = pthread_join(Thread_Periodic.thobj, NULL);
	if (rc != 0) {
	   printf("ERROR: return code from cyclic pthread_join() is %d\n", rc);
	   exit(-1);
	}

	printf("Cyclic Thread Test shutdown complete.\n");
	return EXIT_SUCCESS;
}
