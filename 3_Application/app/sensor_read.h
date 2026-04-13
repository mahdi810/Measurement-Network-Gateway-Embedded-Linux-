/*
 * sensor_read.h
 *
 *  Created on: Dec 9, 2025
 *      Author: mahdi
 */

#ifndef SENSOR_READ_H_
#define SENSOR_READ_H_

#include <stdint.h>

unsigned int MaxRead(int fd, unsigned int mreg);

void MaxWrite(int fd, unsigned int mreg, unsigned int msend);

void GetADC(int fd, unsigned int *adc0_out, unsigned int *adc1_out);

unsigned int ReadSwitch(int fd);

unsigned int ReadButton(int fd);

int TempPowerUp(int fd);

int TempRead(int fd, int *output_raw);

void TempShutdown(int fd);


#endif /* SENSOR_READ_H_ */
