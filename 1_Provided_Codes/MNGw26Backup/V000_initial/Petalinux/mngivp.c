/*
 * @file   mngivp.c  - MMNGWY hardware and drivers test
 * @author Kai Mueller, minor errors removed
 * @version 0.2
 * @brief  A Linux user space program that communicates with the meascdd.c LKM. It passes a
 * string to the LKM and reads the response from the LKM. For this example to work the device
 * must be called /dev/meascdd.
*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>

#include "measdev.h"

// MEAScdd IO data
static MeasObj_struct TF_Obj_Snd, TF_Obj_Rcv;

#define CBUF_LEN 64
static char C_Buf[CBUF_LEN];


static unsigned int MaxRead(int fd, unsigned int mreg)
{
	unsigned int xdata;
	int k;

	mreg <<= 8;
	TF_Obj_Snd.rnum = 1;
	TF_Obj_Snd.rvalue = mreg;
	write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
	k = 0;
	xdata = 0xc0000000;
	do {
		TF_Obj_Snd.rnum = REGNUM_ID;
		TF_Obj_Snd.rvalue = 1;
		write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
		k++;
		read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
		xdata = TF_Obj_Rcv.rvalue;
	} while (((xdata & 0x01) == 0) && (k < 1000));
	if (k >= 1000) {
		printf(" *** MaxRead timeout.");
	} else {
		TF_Obj_Snd.rnum = REGNUM_ID;
		TF_Obj_Snd.rvalue = 2;
		write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
		read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
		xdata = TF_Obj_Rcv.rvalue;
		xdata &= 0x0ff;
		// printf("loop count: %d |  data: %x\n\r", k, xdata);
	}
	return xdata;
}

static void MaxWrite(int fd, unsigned int mreg, unsigned int msend)
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
	do {
		TF_Obj_Snd.rnum = REGNUM_ID;
		TF_Obj_Snd.rvalue = 1;
		write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
		k++;
		read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
		xdata = TF_Obj_Rcv.rvalue;
	} while (((xdata & 0x01) == 0) && (k < 1000));
	if (k >= 1000) {
		printf(" *** MaxWrite timeout.");
	}
}



static void TempMeasure(int fd, int repeatc)
{
	unsigned int xstatus, xdata, xlsb, xadc, k;
	int  ccount;
	double  mresist, mtemp;

	TF_Obj_Snd.rnum = 0;
	TF_Obj_Snd.rvalue = 0x022;
	write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
	// power up
	MaxWrite(fd, 0, 0x081);
	printf("PMB1 initialized...\n");
	printf("  k    raw   decimal   resistance   temperature\n");
	printf("-----------------------------------------------\n");
	for (k = 0; k < repeatc; k++) {
		MaxWrite(fd, 0, 0x0a1);
		ccount = 0;
		do {
			TF_Obj_Snd.rnum = REGNUM_ID;
			TF_Obj_Snd.rvalue = 1;
			write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
			ccount++;
			read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
			xstatus = TF_Obj_Rcv.rvalue;
			xstatus &= 0x03;
		} while ((xstatus == 0x03) && (ccount < 10000000));
		// printf("ccount: %6d\n\r", ccount);
		read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
		xstatus = TF_Obj_Rcv.rvalue;
		xstatus &= 0x03;
		xdata = 0xc0000000;
		if ((xstatus & 0x03) != 0x01) {
			printf(" *** status error: %d\n\r", xstatus);
		} else {
			xdata = MaxRead(fd, 1);
			/*
			TF_Obj_Snd.rnum = REGNUM_ID;
			TF_Obj_Snd.rvalue = 1;
			write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
			read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
			xstatus = TF_Obj_Rcv.rvalue;
			printf("status after read: %x\n\r", xstatus);
			*/
			xdata <<= 8;
			xlsb = MaxRead(fd, 2);
			xdata |= xlsb & 0x0ff;
			xadc = xdata >> 1;
			mresist = xadc * 0.01220703125;
			mtemp = xadc * 0.03125 - 256.0;
		}
		printf("%3d   %4x     %5d   %10.5f    %10.5f\n\r", k, xdata, xadc, mresist, mtemp);
	}
	printf("-----------------------------------------------\n");
	MaxWrite(fd, 0, 0x001);
	printf("PMB1 power down...\n");
}




static void GetADC(int fd, int repeatc)
{
	int  k, ccount;
	unsigned int xstatus, adc0, adc1;

	printf("  k    adc0      adc1    (retries)\n");
	printf("----------------------------------\n");
	for (k = 0; k < repeatc; k++) {
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
		printf("%3d    %4x     %4x     (%d)\n", k, adc0, adc1, ccount);
	}
	printf("----------------------------------\n");
}



int main()
{
	int retc, fd;
	unsigned int cregn, cregval;
	int  terminate_me, repeatc;

	printf("Starting MNG IVP... (on rootfs)\n");
	fd = open("/dev/meascdd", O_RDWR);			// Open the device with read/write access
	if (fd < 0) {
		perror("Failed to open the device...");
		return errno;
	}
	// initialize send object
	TF_Obj_Snd.rnum = 0;
	TF_Obj_Snd.rvalue = 0;

	terminate_me = 0;
	do {
		printf(">> "); fflush(stdout);
		fgets(C_Buf, CBUF_LEN, stdin);
		C_Buf[CBUF_LEN-1] = '\0';
		if (C_Buf[0] == 'x') {
			terminate_me = 1;
		} else if (C_Buf[0] == 'h') {
			printf("MNG test commands\n");
			printf("  h           - Help (this command)\n");
			printf("  p           - read register R0 (pushbuttons & switches)\n");
			printf("  r{n}        - read register n (0 <= n <= 7)\n");
			printf("  w{n} {val}  - write val to register n\n");
			printf("  i{0|1}      - disable | enable interrupt\n");
			printf("  t{n}        - temperature measurement, n is number of samples\n");
			printf("  a{n}        - adc acquisition, n is number of samples\n");
			printf("  ----------\n");
		} else if (C_Buf[0] == 'p') {
			TF_Obj_Snd.rnum = REGNUM_ID;
			TF_Obj_Snd.rvalue = 0;
			printf(" o R0 set for reading.\n");
			retc = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);	// Write to LKM
			if (retc < 0) {
				perror("Failed to write message to device.");
				close(fd);
				return errno;
			}
			retc = read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
			if (retc < 0) {
				perror("Failed to read message from device.");
				close(fd);
				return errno;
			} else {
				printf("PB: %2x  SW: %2x (hex)\n", TF_Obj_Rcv.rvalue >> 8, TF_Obj_Rcv.rvalue & 0x0ff);
			}
		} else if (C_Buf[0] == 'r') {
			if (sscanf(&C_Buf[1], "%d", &cregn) != 1) {
				printf("*** error converting int arg.\n");
			} else {
				TF_Obj_Snd.rnum = REGNUM_ID;
				TF_Obj_Snd.rvalue = cregn;
				printf(" o R%d set for reading.\n", cregn);
				retc = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);	// Write to LKM
				if (retc < 0) {
					perror("Failed to write message to device.");
					close(fd);
					return errno;
				}
				retc = read(fd, &TF_Obj_Rcv, MEASOBJ_SIZE);		// Read response from LKM
				if (retc < 0) {
					perror("Failed to read message from device.");
					close(fd);
					return errno;
				}
				printf("R%d: %x (hex)\n", TF_Obj_Rcv.rnum, TF_Obj_Rcv.rvalue);
			}
		} else if (C_Buf[0] == 'w') {
			if (sscanf(&C_Buf[1], "%d %x", &cregn, &cregval) != 2) {
				printf("*** error converting int.\n");
			} else {
				if (cregn >= NREGS) {
					printf("*** illegal register\n");
				} else {
					TF_Obj_Snd.rnum = cregn;
					TF_Obj_Snd.rvalue = cregval;
					printf(" o sending %x to REG[%d]\n", cregval, cregn);
					retc = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
					if (retc < 0) {
						perror("Failed to write the message to the device.");
						close(fd);
						return errno;
					}
				}
			}
		} else if (C_Buf[0] == 'i') {
			if (C_Buf[1] == '0') {
				printf(" => disable interrupt.\n");
				TF_Obj_Snd.rnum = 3;
				TF_Obj_Snd.rvalue = TIMER100ms;
				retc = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
				if (retc < 0) {
					perror("Failed to write the message to the device.");
					close(fd);
					return errno;
				}
			} else if (C_Buf[1] == '1') {
				printf(" => enable interrupt.\n");
				TF_Obj_Snd.rnum = 3;
				TF_Obj_Snd.rvalue = MBINT_ENABLE | TIMER100ms;
				retc = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
				if (retc < 0) {
					perror("Failed to write the message to the device.");
					close(fd);
					return errno;
				}
			}
		} else if (C_Buf[0] == 't') {
			repeatc = 0;
			if ((C_Buf[1] == '\0') || (C_Buf[1] == '\n')) {
				repeatc = 1;
			} else if (sscanf(&C_Buf[1], "%d", &repeatc) != 1) {
				printf("*** error converting int.\n");
			}
			if (repeatc != 0) {
				TempMeasure(fd, repeatc);
			}
		} else if (C_Buf[0] == 'a') {
			repeatc = 0;
			if ((C_Buf[1] == '\0') || (C_Buf[1] == '\n')) {
				repeatc = 1;
			} else if (sscanf(&C_Buf[1], "%d", &repeatc) != 1) {
				printf("*** error converting int.\n");
			}
			if (repeatc != 0) {
				GetADC(fd, repeatc);
			}
		} else {
			printf("*** unknown command\n");
		}
	} while (terminate_me == 0);

	printf(" => shutdown.\n");
	TF_Obj_Snd.rnum = 3;
	TF_Obj_Snd.rvalue = TIMER100ms;
	retc = write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
	if (retc < 0) {
		perror("Failed to write the message to the device.");
		close(fd);
		return errno;
	}
	TF_Obj_Snd.rnum = 0;
	TF_Obj_Snd.rvalue = 0;
	write(fd, &TF_Obj_Snd, MEASOBJ_SIZE);
	if (retc < 0) {
		perror("Failed to write the message to the device.");
		close(fd);
		return errno;
	}
	close(fd);
	printf("Thank you for using mngtest.\n");
	return 0;
}

