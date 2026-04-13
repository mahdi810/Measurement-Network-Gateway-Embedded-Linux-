/*
 * sensor_read.c
 *
 *  Created on: Dec 9, 2025
 *      Author: mahdi
 */

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "Ring_Buffer.h"
#include "measdev.h"
#include "sensor_read.h"

// Real hardware mode - use external functions and variables
MeasObj_struct TF_Obj_Snd, TF_Obj_Rcv;

// the maxread function, for reading commands into temperature sensor.
unsigned int MaxRead(int fd, unsigned int mreg)
{
    unsigned int xdata;
    int k;

    mreg <<= 8;
    TF_Obj_Snd.rnum = 1;
    TF_Obj_Snd.rvalue = mreg;
    write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
    k = 0;
    xdata = 0xc0000000;
    do
    {
        TF_Obj_Snd.rnum = REGNUM_ID;
        TF_Obj_Snd.rvalue = 1;
        write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
        k++;
        read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE); // Read response from LKM
        xdata = TF_Obj_Rcv.rvalue;
    } while (((xdata & 0x01) == 0) && (k < 1000));
    if (k >= 1000)
    {
        printf(" *** MaxRead timeout.");
    }
    else
    {
        TF_Obj_Snd.rnum = REGNUM_ID;
        TF_Obj_Snd.rvalue = 2;
        write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
        read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE); // Read response from LKM
        xdata = TF_Obj_Rcv.rvalue;
        xdata &= 0x0ff;
        // printf("loop count: %d |  data: %x\n\r", k, xdata);
    }
    return xdata;
}

// the function for writting the values from the temperature sensor.
void MaxWrite(int fd, unsigned int mreg, unsigned int msend)
{
    unsigned int transm_data, xdata;
    int k;

    mreg |= 0x080;
    transm_data = (mreg << 8) | (msend & 0x0ff);
    TF_Obj_Snd.rnum = 1;
    TF_Obj_Snd.rvalue = transm_data;
    write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
    k = 0;
    xdata = 0xc0000000;
    do
    {
        TF_Obj_Snd.rnum = REGNUM_ID;
        TF_Obj_Snd.rvalue = 1;
        write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
        k++;
        read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE); // Read response from LKM
        xdata = TF_Obj_Rcv.rvalue;
    } while (((xdata & 0x01) == 0) && (k < 1000));
    if (k >= 1000)
    {
        printf(" *** MaxWrite timeout.");
    }
}


//--------------------------------------------------------------------------------------------------------

// function to get the values of ADC0 and ADC1
void GetADC(int fd, unsigned int *adc0_out, unsigned int *adc1_out)
{
	int  ccount;
	unsigned int xstatus, adc0, adc1;

		TF_Obj_Snd.rnum = 4;		// start conversion
		TF_Obj_Snd.rvalue = 0;
		write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
		ccount = 0;
		do {
			ccount++;
			read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
			xstatus = TF_Obj_Rcv.rvalue;
		} while ((xstatus == 0) && (ccount < 10000000));
		TF_Obj_Snd.rnum = REGNUM_ID;
		TF_Obj_Snd.rvalue = 5;		// ADC status register
		write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
		read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
		adc1 = TF_Obj_Rcv.rvalue;
		adc0 = adc1 & 0x0fff;
		adc1 >>= 16;
		*adc0_out = adc0;
		*adc1_out = adc1;
}

//---------------------------------------------------------------------------------------------------

// function to read the switches.
unsigned int ReadSwitch(int fd)
{
    // Read switch register
    TF_Obj_Snd.rnum = REGNUM_ID;
    TF_Obj_Snd.rvalue = 0;

    // requesting the LKM for reading the switch from zedboard.
    write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);

    // getting the read data from the LKM
    read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);

    return (TF_Obj_Rcv.rvalue) & 0xff;
}

// function to read the pushbuttons from zedboard.
unsigned int ReadButton(int fd)
{
    // Read button register
    TF_Obj_Snd.rnum = REGNUM_ID;
    TF_Obj_Snd.rvalue = 0;
    // reqeusting the LKM for reading the data.
    write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);

    // getting the values from the LKM.
    read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);

    return ((TF_Obj_Rcv.rvalue) >> 8) & 0x1f;
}
 
void LedWrite(int fd, uint8_t LED_hex_Value)
{
    ssize_t ret;

    TF_Obj_Snd.rnum   = 0;       // write reg0 (lower 8 bits = uzLED)
    TF_Obj_Snd.rvalue = (unsigned int)LED_hex_Value;
    ret = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
    if (ret < 0) perror("LED writing failed");
}


int TempPowerUp(int fd)
{
    ssize_t ret;

    TF_Obj_Snd.rnum   = 0;
    TF_Obj_Snd.rvalue = 0x022;
    ret = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
    if (ret < 0) { perror("Sensor Initialization Failed"); return -1; }

    MaxWrite(fd, 0x00, 0xC1);   // VBIAS=ON, AutoConv=ON, 2-wire, 50Hz
    usleep(75 * 1000);           // 10ms VBIAS settle + 65ms first conversion
    printf("MAX31865 powered up in auto-conversion mode\n");
    return 1;
}

int TempRead(int fd, int *output_raw)
{
    unsigned int msb = MaxRead(fd, 0x01);
    unsigned int lsb = MaxRead(fd, 0x02);

    if (lsb & 0x01) {
        printf("MAX31865 fault: 0x%02X\n", MaxRead(fd, 0x07));
        return -1;
    }

    *output_raw = (int)((((msb & 0xFF) << 8) | (lsb & 0xFF)) >> 1);
    return 1;
}

void TempShutdown(int fd)
{
    MaxWrite(fd, 0x00, 0x01);
    printf("MAX31865 powered down\n");
}

