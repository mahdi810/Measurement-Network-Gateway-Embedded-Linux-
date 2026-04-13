/*
 * Ring_Buffer.h
 *
 *  Created on: Dec 9, 2025
 *      Author: mahdi
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

/*
 * Ring_Buffer.h
 *
 *  Created on: Nov 15, 2025
 *      Author: root
 */


#include <stdint.h>
#include <pthread.h>
#include "measdev.h"

/* -------------------- Configuration -------------------- */
#define RBUF_LEN   2048    // Ring buffer length
#define N_SENSORS  5     // Number of sensors

/* -------------------- Ring Buffer Structure -------------------- */
typedef struct {
    pthread_mutex_t bmutex;            // Mutex for thread safety
    volatile int sensor_id[RBUF_LEN];  // Sensor IDs
    volatile int rb_pointer;           // Write pointer
    volatile int rb_cnt;               // Number of valid entries
    volatile uint64_t tstamp[RBUF_LEN]; // Timestamp of each sample
    volatile int svalue[RBUF_LEN];     // Sensor values
} RingB_Struct;

/* -------------------- Global Buffer Instance -------------------- */
extern RingB_Struct Ring_Buf;

/* -------------------- Function Prototypes -------------------- */

/**
 * @brief Initialize the timer
 *        Initializes the timer for timestamp tracking
 */
void InitTimer(void);

/**
 * @brief Initialize the ring buffer
 *        Initializes mutex, sets pointer and count to 0
 */
void InitRingBuffer(void);

/**
 * @brief Push a new sensor sample into the ring buffer
 * @param sid  Sensor ID
 * @param ts   Timestamp of the sample
 * @param sv   Sensor value
 */
void PushRB(int sid, uint64_t ts, int sv);

/**
 * @brief Pop the oldest sample from the ring buffer
 * @param sid  Pointer to store Sensor ID
 * @param ts   Pointer to store Timestamp
 * @param sv   Pointer to store Sensor Value
 * @return 0 if a sample was popped, -1 if buffer is empty
 */
int PopRB(int *sid, uint64_t *ts, int *sv);

//returns the elapsed time in microseconds since startup
uint64_t GetElapsedTime(void);

unsigned int MaxRead(int fd, unsigned int mreg);

void MaxWrite(int fd, unsigned int mreg, unsigned int msend);

#endif /* RING_BUFFER_H_ */
