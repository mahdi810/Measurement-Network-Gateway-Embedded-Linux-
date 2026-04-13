/*
 * measdev.h
 *
 *  Created on: Dec 9, 2025
 *      Author: mahdi
 */

#ifndef MEASDEV_H_
#define MEASDEV_H_

// MEAScdd IO data
typedef struct {
    unsigned int rnum;
    unsigned int rvalue;
}  MeasObj_struct;

#define MEASOBJ_SIZE sizeof(MeasObj_struct)
#define REGNUM_ID 0x0FF
#define NREGS 8

#define TIMER100ms  0x009896ff
#define MBINT_ENABLE 0x80000000
#define MBINT_ACKN 0x40000000

#endif /* MEASDEV_H_ */
