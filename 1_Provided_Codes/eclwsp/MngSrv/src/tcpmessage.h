/*
 * tcpmessage.h
 *
 *  Created on: Sep 21, 2022
 *      Author: mueller
 */

#ifndef TCPMESSAGE_H_
#define TCPMESSAGE_H_

#define PORT 50012

#define MAX_SAMPLES  10
typedef struct {
	unsigned int msg_type;
	unsigned int nsamples;
	unsigned int scount;
	unsigned int stime[MAX_SAMPLES];
	unsigned int sval[MAX_SAMPLES];
} Srv_Send_struct;

#define MAX_PARAMS  3
typedef struct {
	unsigned int cmd;
	unsigned int param[MAX_PARAMS];
} Clnt_Send_struct;


#endif /* TCPMESSAGE_H_ */
