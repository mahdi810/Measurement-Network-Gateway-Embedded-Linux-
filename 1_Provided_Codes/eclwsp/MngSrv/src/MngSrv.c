/*
** ============================================================================
** Name        : MngSrv.c
** Author      : Kai Mueller
** Version     :
** Copyright   : K. Mueller, 21-SEP-2022
** Description : TCP sever for measurement data simulation
** ============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "tcpmessage.h"

#define PORT 50012


static struct timespec Current_Time, Start_Time;
static uint64_t us_start_time;
static Srv_Send_struct Meas_Data;
static Clnt_Send_struct Clnt_Data;

static int Sample_Count;

static pthread_t  TcpSrvTHR;

static int Sign_Gen_idx;
#define SINCOS_PER  200
#define SIGN_GEN_DELTA 31.83098862


// get time in us
static unsigned int GetElapsedTime()
{
	uint64_t us_current_time;
	unsigned int ret_time;

	clock_gettime(CLOCK_MONOTONIC, &Current_Time);
	us_current_time = Current_Time.tv_sec * 1000000 + Current_Time.tv_nsec / 1000;
	ret_time = us_current_time - us_start_time;
	return ret_time;
}


static void *TcpServerF(void *sockfd_ptr)
{
	int sockfd;
	int connfd;
	unsigned int len;
	int  y0, y1;
	struct sockaddr_in cli;
	unsigned int clnt_msg_size, srv_msg_size, n_read;

	clnt_msg_size = sizeof(Clnt_Send_struct);
    srv_msg_size =  sizeof(Srv_Send_struct);

	sockfd = *(int *)sockfd_ptr;
	printf("%%TcpServerF: %d\n", sockfd);

	// Accept the data packet from client and verification
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) {
		printf("server acccept failed...\n");
		exit(1);
	} else {
		printf("server acccept the client...\n");
	}

	for (;;) {
		// read the message from client and copy it in buffer
		bzero(&Clnt_Data, clnt_msg_size);
		n_read = read(connfd, &Clnt_Data, clnt_msg_size);
		// print buffer which contains the client contents
		if (n_read == 0) {
			printf("* no data *\n");
			break;
		} else {
			printf("cmd: %d\n", Clnt_Data.cmd);
			printf(" P0: %d\n", Clnt_Data.param[0]);
			printf(" P1: %d\n", Clnt_Data.param[1]);
			printf(" P3: %d\n", Clnt_Data.param[2]);

			Meas_Data.msg_type = 42;
			Meas_Data.nsamples = 2;
			Meas_Data.scount = Sample_Count;
			Meas_Data.stime[0] = GetElapsedTime();
			y0 = round(30000.0 * sin(SIGN_GEN_DELTA * Sign_Gen_idx));
			Meas_Data.sval[0] = *(unsigned int *)&y0;
			Meas_Data.stime[1] = GetElapsedTime();
			y1 = round(30000.0 * cos(SIGN_GEN_DELTA * Sign_Gen_idx));
			Meas_Data.sval[1] = *(unsigned int *)&y1;
			Meas_Data.stime[2] = GetElapsedTime();
			Meas_Data.sval[2] = 112;
			Meas_Data.stime[3] = GetElapsedTime();
			Meas_Data.sval[3] = 223;
			Sign_Gen_idx++;
			Sample_Count++;
		// printf("Sending [%s] (%d)\n", buff, (int)strlen(buff));
		// write(connfd, buff, strlen(buff));
		write(connfd, &Meas_Data, srv_msg_size);
		// printf("Sent: [%s] (%d)\n", buff, (int)strlen(buff));
		}
	}
	pthread_exit(NULL);
}




int main(int argc , char *argv[])
{
	int rc;
	int sockfd;
	struct sockaddr_in servaddr;
	void *status;

	Sign_Gen_idx = 0;
	Meas_Data.msg_type = 0;
	Meas_Data.nsamples = 0;
	Meas_Data.scount = 0;
	Sample_Count = 0;
	clock_gettime(CLOCK_MONOTONIC, &Start_Time);
	us_start_time = Start_Time.tv_sec * 1000000 + Start_Time.tv_nsec / 1000;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	} else {
		printf("Socket successfully created..\n");
	}

	// assign IP, PORT
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	} else {
		printf("Socket successfully binded..\n");
	}

	for (;;) {
		printf("accepting connection...\n");
		// Now server is ready to listen and verification
		if ((listen(sockfd, 5)) != 0) {
			printf("Listen failed...\n");
			exit(0);
		} else {
			printf("Server listening..\n");
		}

		// thread for TCP server
		printf("sockfd: %d\n", sockfd);
		rc = pthread_create(&TcpSrvTHR, NULL, TcpServerF, &sockfd);
	    if (rc != 0) {
	        printf("ERROR; return code from pthread_create() is %d\n", rc);
	        exit(-1);
	     }

		printf(" o waiting for thread joins...\n");
	    rc = pthread_join(TcpSrvTHR, &status);
	    if (rc != 0) {
	       printf("ERROR: return code from pthread_join() is %d\n", rc);
	       exit(-1);
	    }
	    printf("Client disconnected...\n\n");
	}

	// close socket
	close(sockfd);
	printf("MngSrv shutdown complete.\n");
	return EXIT_SUCCESS;
}
