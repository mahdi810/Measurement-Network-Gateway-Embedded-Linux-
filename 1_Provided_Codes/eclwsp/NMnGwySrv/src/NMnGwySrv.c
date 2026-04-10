/*
 ============================================================================
 Name        : NMnGwySrv.c
** Author      : Kai Mueller
** Version     :
** Copyright   : K. Mueller, 05-DEC-2020
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

#define BUFFER_SIZE 80
#define PORT 50012

static struct timespec Startup_Time, Current_Time;
static uint64_t US_Startup_Time;

static double GetElapsedTime()
{
	double etime;
	uint64_t us_current_time;

	clock_gettime(CLOCK_MONOTONIC, &Current_Time);
	us_current_time = Current_Time.tv_sec * 1000000 + Current_Time.tv_nsec / 1000;
	etime = 1.0e-6 * (us_current_time - US_Startup_Time);
	return  etime;
}


typedef struct {
	int sample_c;
	double mtime;
	int  mval;
} Measurement_struct;


static Measurement_struct Temp_1, Temp_2;


static pthread_t  TcpSrvTHR;

static void *TcpServerF(void *sockfd_ptr)
{
	int sockfd;
	char buff[BUFFER_SIZE];
	int connfd;
	unsigned int len;
	struct sockaddr_in cli;

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
		bzero(buff, BUFFER_SIZE);
		read(connfd, buff, sizeof(buff));
		// print buffer which contains the client contents
		// printf("From client: [%s]\n", buff);
		if (strlen(buff) == 0) {
			break;
		} else if (strncmp(buff, "#T1", 3) == 0) {
			Temp_1.mval = round(200.0 * sin(0.6284 * Temp_2.sample_c++));
			if (Temp_1.sample_c >= 30000) {
				Temp_1.sample_c = 0;
			}
			Temp_1.mtime = GetElapsedTime();
			bzero(buff, BUFFER_SIZE);
			sprintf(buff, "~T1: %12.5f %d\n", Temp_1.mtime, Temp_1.mval);
			// printf("==T0==> [%s]", buff);
		} else if (strncmp(buff, "#T2", 3) == 0) {
			Temp_2.mval = round(150.0 * cos(0.6284 * Temp_2.sample_c++));
			if (Temp_2.sample_c >= 30000) {
				Temp_2.sample_c = 0;
			}
			Temp_2.mtime = GetElapsedTime();
			bzero(buff, BUFFER_SIZE);
			sprintf(buff, "~T2: %12.5f %d\n", Temp_2.mtime, Temp_2.mval);
			// printf("==T1==> [%s]", buff);
		} else {
			strcpy(buff, "*req*\n");
		}

		// printf("Sending [%s] (%d)\n", buff, (int)strlen(buff));
		write(connfd, buff, strlen(buff));
		// printf("Sent: [%s] (%d)\n", buff, (int)strlen(buff));
	}
	pthread_exit(NULL);
}




int main(int argc , char *argv[])
{
	int rc;
	int sockfd;
	struct sockaddr_in servaddr;
	void *status;

	Startup_Time.tv_sec = 0;
	Startup_Time.tv_nsec = 0;
	if (clock_gettime(CLOCK_MONOTONIC, &Startup_Time) == 0) {
		printf("Start_Time received.\n");
	} else {
		printf("Error getting CLOCK_MONOTIC time.\n");
	}
	US_Startup_Time = Startup_Time.tv_sec * 1000000 + Startup_Time.tv_nsec / 1000;

	Temp_1.sample_c = 0;
	Temp_1.mtime = 0.0;
	Temp_1.mval = 0;
	Temp_2.sample_c = 0;
	Temp_2.mtime = 0.0;
	Temp_2.mval = 0;

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
