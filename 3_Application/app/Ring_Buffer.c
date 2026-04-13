/*
 * Ring_Buffer.c
 *
 *  Created on: Dec 9, 2025
 *      Author: mahdi
 */


#include "Ring_Buffer.h"
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// ----------------------------
// Global ring buffer
// ----------------------------
RingB_Struct Ring_Buf;

// ----------------------------
// Timer variables
// ----------------------------
static struct timespec Current_time;
static uint64_t US_startup_time;

// ----------------------------
// Initialize timer
// ----------------------------
void InitTimer(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        perror("clock_gettime failed");
        exit(EXIT_FAILURE);
    }
    US_startup_time = ts.tv_sec * 1000000 + ts.tv_nsec / 1000; // microseconds
}

// ----------------------------
// Get elapsed time since InitTimer
// ----------------------------
uint64_t GetElapsedTime(void)
{
    if (clock_gettime(CLOCK_MONOTONIC, &Current_time) != 0)
    {
        perror("clock_gettime failed");
        return 0;
    }
    uint64_t us_current_time = Current_time.tv_sec * 1000000 + Current_time.tv_nsec / 1000;
    return us_current_time - US_startup_time;
}

// ----------------------------
// Initialize ring buffer
// ----------------------------
void InitRingBuffer(void)
{
    pthread_mutex_init(&Ring_Buf.bmutex, NULL);
    Ring_Buf.rb_pointer = 0;
    Ring_Buf.rb_cnt = 0;

    for (int i = 0; i < RBUF_LEN; i++)
    {
        Ring_Buf.sensor_id[i] = 0;
        Ring_Buf.svalue[i] = 0;
        Ring_Buf.tstamp[i] = 0;
    }
}

// ----------------------------
// Push a sample into the ring buffer
// ----------------------------
void PushRB(int sid, uint64_t ts, int sv)
{
    pthread_mutex_lock(&Ring_Buf.bmutex);

    Ring_Buf.sensor_id[Ring_Buf.rb_pointer] = sid;
    Ring_Buf.tstamp[Ring_Buf.rb_pointer] = ts;
    Ring_Buf.svalue[Ring_Buf.rb_pointer] = sv;

    if (Ring_Buf.rb_cnt < RBUF_LEN)
    {
        Ring_Buf.rb_cnt++;
    }

    Ring_Buf.rb_pointer = (Ring_Buf.rb_pointer + 1) % RBUF_LEN;

    pthread_mutex_unlock(&Ring_Buf.bmutex);
}

// ----------------------------
// Pop the oldest sample from the ring buffer
// ----------------------------
int PopRB(int *sid, uint64_t *ts, int *sv)
{
    pthread_mutex_lock(&Ring_Buf.bmutex);

    if (Ring_Buf.rb_cnt == 0)
    {
        pthread_mutex_unlock(&Ring_Buf.bmutex);
        return -1; // buffer empty
    }

    int idx = (Ring_Buf.rb_pointer - Ring_Buf.rb_cnt + RBUF_LEN) % RBUF_LEN;

    *sid = Ring_Buf.sensor_id[idx];
    *ts = Ring_Buf.tstamp[idx];
    *sv = Ring_Buf.svalue[idx];

    Ring_Buf.rb_cnt--; // remove this sample

    pthread_mutex_unlock(&Ring_Buf.bmutex);

    return 0; // success
}

