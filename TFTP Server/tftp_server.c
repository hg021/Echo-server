/* tftp_server.c
 * ----------------------------------------------------
 * AUTHOR: HIMANSHU GUPTA, ADARSH
 * himgupt2@tamu.edu, msadarsh@tamu.edu
 *
 * ECEN 602 Fall 17
 * TFTP server
 * ----------------------------------------------------
 */


#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netdb.h>
#include<sys/select.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<time.h>
#include<errno.h>
#include<signal.h>
#include<sys/wait.h>

#include"tftp_util.h"

#define START 1
#define STOP 0

/* Global variables to be used by server process*/

char tftp_buf[1000];
char tftp_rbuf[1000];
char nextchar=-1;

int mlen;
int retry= 0;
struct sockaddr_in caddr;
int client_fd, clen;
char tftp_sbuf[1000];
tftp_ack ack;

/*
 * Function to resend the data block if
 * ack is not received.
 */ 

void tftp_resend(int signal) {
	
	int ret;
	printf("Ack not received, retry %d\n", retry);
	if (retry > 10){
		printf("max retry, abort \n");
		close(client_fd);
		exit(1);
	}
	retry++;
	ret = sendto(client_fd, tftp_buf, mlen, 0, (struct sockaddr *)&caddr, clen);
	alarm(START);

	return;
}

/*
 * Function to resend the ack if data is 
 * not received in 1 sec
 */

void tftp_sendack(int signal) {

	int ret;
	printf("Retrying ack, %d\n", retry);
	if (retry > 10){
		printf("Max retry , abort \n");
		close(client_fd);
		exit(1);
	}
	retry++;
	ret = sendto(client_fd, (void *)&ack, 4, 0, (struct sockaddr *)&caddr, clen);
	alarm(START);
	return;
}

/*
 * Function to handle the killed child 
 * process, to avoid the child process in 
 * zombie state
 */

void handle_child(int signal) {
        pid_t pid;
        int child;
        pid = wait(&child);
        fprintf(stdout, "child %d is now terminated\n", pid);
        return;
}

/*
 * Send an error message to client
 * if the request can not be served.
 */

int tftp_send_err_data (
	struct sockaddr_in caddr,
	int clen,
	int client_fd,
	int error)
{
	int ret;
	tftp_err msg;
	msg.code = htons(TFTP_ERROR);
	msg.err_code = htons(error);

	switch(error) {
		case 1:
			sprintf(msg.err_msg,"File not found.");
			break;
		case 6:
			sprintf(msg.err_msg, "File already exists.");
			break;
		default:
			printf("Invalid error\n");
	}
	ret = sendto(client_fd, (void *)&msg, sizeof(msg), 0, (struct sockaddr *) &caddr, clen);
	if (ret < 0) {
                printf("Error in sendto \n ");
                return 0;
        } else {
                return 1;
        }

}

/* 
 * Module to send a block of data 
 * to client, takes the following input
 */

int tftp_send_block_data (
	FILE **file,			/* File pointer which is being sent*/
	struct sockaddr_in caddr,	/* Client address structure */
	int clen,			/* Address len */
	int client_fd,			/* Socket FD to use for sending datagram */
	int bnum,			/* Block number to set in data packet */
	int if_netascii)    		/* checks if netascii file or not 1-yes 0-octet */
{
//here_point
   	int i;
	int ret, read;
	int loopmax;
	char chartmp;
	tftp_data msg;

	msg.code = htons(TFTP_DATA);
	msg.bnum = htons(bnum);
	//printf("block number %d\n", msg.bnum);
	
	//MODIFICATIONS
	/* Logic to handle netascii files */
	if(if_netascii==1){
		for(i=0;i<BLOCK_SIZE;i++){
	
			if(nextchar>=0){
      				msg.data[i]=nextchar;
      				nextchar=-1;	
				continue;	  
			}
	
		chartmp=getc(*file);
	
		if(feof(*file)){
	  		break;
		}
	
		else if(chartmp=='\n'){
			chartmp='\r';
			nextchar='\n';
	
		}
	
		else if(chartmp=='\r'){
		nextchar='\0';
	
		}
		msg.data[i]=chartmp;
		}
	} else {
		for(i=0;i<BLOCK_SIZE;i++){
			chartmp=getc(*file);
	
			if(!feof(*file)){
				msg.data[i]=chartmp;
			} else {
				break;
			}
		}
	
	}


	mlen = i+4;			/* Set the message len to be used in retry */

	memset(tftp_buf, 0, 1000);
	/* 
	 * copy the message in global array to use for resend
	 * This is a buffer maintened in UDP applications to 
	 * resend the packet if the corresponding ack is not 
	 * in waiting time.
	 */
	
	memcpy(tftp_buf, (char *)&msg, mlen);

	/* Now send the datagram to client */
	ret = sendto(client_fd, tftp_buf, mlen, 0, (struct sockaddr *) &caddr, clen);
        
}

/* 
 * A helper function to create a UDP socket
 * and bind it to a local address, this will 
 * return the file descriptor to the calling 
 * api
 */

int get_udp_sock() {

	struct sockaddr_in caddr;
	int fd, port;

	/* create a new datagram socket */
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd<0) {
		perror("Failed to open socket\n");
		exit(1);
	}

	int options = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof (options));

	memset(&caddr, 0, sizeof(struct sockaddr_in));

	caddr.sin_family = AF_INET;
	caddr.sin_addr.s_addr = htonl(INADDR_ANY);
	caddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&caddr, sizeof(caddr))) {
                perror("Failed to bind socket");
                exit(1);
        }
	printf("new client bind success\n");
	return fd;

}

/*
 * Method to handle the RRQ request from 
 * client. Takes in the read buffer as one of the 
 * input and extracts the file to be sent
 */
 

int tftp_handle_rrq(char *buf, struct sockaddr_in addr, int clen, int fd) {

	char file[512];
	char mode[15];
	int n, ret, len;
	FILE *tftp_file;
	char *ip;
	int client_fd;
	int bnum = 1;
	int if_netascii=0; // 1- netascii, 0- octet 

	memcpy(file, buf + sizeof(uint16_t), 512);
	len = strlen(file);
	
	memcpy(mode,buf+sizeof(uint16_t)+len+1,15);
    
	if(!strcmp(mode,"netascii")) {

		if_netascii=1;
		printf("Mode is Netascii\n");
	} else {
		printf("Mode is binary \n");
		if_netascii=0;
	}
    
    
	ip = inet_ntoa(addr.sin_addr);
	if(!ip) {
		printf("Invalid address\n");
	}

	printf("Got RRQ from %s for %s\n", ip, file);

	/* OPen the file to be sent in read mode */	
	tftp_file = fopen(file, "rb");
	
	/* 
	 * Send the error message if we do not find this file
	 * Return from here itself.
	 */

	if (!tftp_file) {
		printf("File not found \n");
		tftp_send_err_data(addr, clen, fd, 1);
		return -1;
	}

	/*  
	 * create a new fd to talk to this client 
	 */

	client_fd = get_udp_sock();

	/* Close the actual server FD in the child process */
	close(fd);

	/* Read the file in a loop */
	while(!feof(tftp_file)) {

		memset (tftp_rbuf, 0, 1000);
		/* Setup the alarm handle */
		signal(SIGALRM, tftp_resend);
		ret = tftp_send_block_data(&tftp_file, addr, clen,
                      	          client_fd, bnum,if_netascii);

		bnum++;
		bnum = bnum %65536; /*wrap around for tftp */

		/* Start the alarm before reading the packet from network */
		alarm(START);

		ret = recvfrom(client_fd, tftp_rbuf, 1000, 0, (struct sockaddr *)&caddr, &clen);
		if (ret < 0) {
			printf("Error in recvfrom\n");
			if (errno == EINTR)
				printf("Timeout for client %d\n", client_fd);
		} else {
			/* Reset the retry parameters */
			retry = 0;
			/* Stop the alarm if we receive the packet */
			alarm(STOP);
		}

		/* break out of the loop when we hit a packet smaller than 516 */
		if (mlen < 516)
			break;

	
	}
	/* Close the file pointer */
	fclose(tftp_file);
	return client_fd;
	
}

/* 
 * Method to handle the incoming write(WRQ) requests
 */

int tftp_handle_wrq(char *buf, struct sockaddr_in addr, int clen, int fd) {

	uint16_t bnum = 0;
//	tftp_ack ack;
	int ret;
	FILE *tftp_file;
	char file[512];
	uint16_t code;
	int if_netascii, i;
	char mode[15];
	char chartmp;
   
	/* Set the ack parameters */
	ack.code = htons(TFTP_ACK);
	ack.bnum = htons(bnum);

	close(fd);		/* Close the server fd in child process*/
	client_fd = get_udp_sock();

	/* Extract the file name to be writen on server */
	memcpy(file, buf + sizeof(uint16_t), 512);

	if (fopen(file,"r")) {
                printf("file exists- %s, send error\n", file);
                tftp_send_err_data(addr, clen, client_fd, 6);
                return 0;
        }

	/* Send the first ack message with 0 block num*/

	ret = sendto(client_fd, (void *)&ack, 4, 0, (struct sockaddr *)&caddr, clen);
	if (ret<0 || ret != 4) {
		printf("Ack not sent for incoming file");
	}
	
	memcpy(mode,buf + sizeof(uint16_t) +strlen(file)+1,15);
	
	if(!strcmp(mode,"netascii")) {
		if_netascii=1;
		printf("mode is netascii \n");
	} else {
		if_netascii=0;
		printf("Mode is binary \n");
	}

	/* Set the signal function */
	signal(SIGALRM, tftp_sendack);

	/* Write the incoming file*/
	tftp_file = fopen(file, "wb");
	printf("Writing file %s\n", file);


	while(1) {
		
		memset(tftp_rbuf, 0, 1000);
		alarm(START);
		ret = recvfrom(client_fd, tftp_rbuf, 1000, 0, (struct sockaddr *)&caddr, &clen);
		if (ret < 0) {
			printf("Error in recieve abort\n");
			break;
		} else {
			retry = 0;
			alarm(STOP);
		}
		memcpy(&code, tftp_rbuf, sizeof(uint16_t));
		code = ntohs(code);
		if (code != TFTP_DATA){
			printf("Invalid opcode\n");
		}
		memcpy(&bnum, tftp_rbuf+ sizeof(uint16_t), sizeof(uint16_t));
		bnum = ntohs(bnum);
	
		/* Write the incoming data in the local copy */
		fwrite(tftp_rbuf+4,1, ret-4, tftp_file);
		
		/* Update the ack number to be sent */
		ack.bnum = htons(bnum);

		/* Send the ack number to be sent to client */
		sendto(client_fd, (void *)&ack, 4, 0, (struct sockaddr *)&caddr, clen);

		/* break condition if message size is smaller than 516 */
		if (ret < 516)
			break;
	}
	/* close the file pointer */
	fclose(tftp_file);
	return client_fd;
}

int main (int argc, char **argv)
{

	int server_fd;
	int i, ret;
	struct sockaddr_in s_addr;
	int fd;
	int read;
	char buf[1000];

	struct hostent *server_ip;

	if (argc!= 3){
		printf("Invalid arg specify ./server <IP> <port>");
		exit(1);
	}

	int port = htons(atoi(argv[2]));
	clen = sizeof(caddr);


	/* create the datagram server socket */
	server_fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (server_fd < 0){
		perror("Failed to open server server socket");
		exit(0);
	}
	printf("Server socket created %d\n", server_fd);

	 /* Add socket reuse options to avoid error while starting*/
        int options = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof (options));


	memset(&s_addr, 0, sizeof(struct sockaddr_in));

        s_addr.sin_family = AF_INET;
        server_ip = gethostbyname(argv[1]);
        memcpy(&s_addr.sin_addr.s_addr, server_ip->h_addr, server_ip->h_length);
        s_addr.sin_port = port;

	 /* Bind the socket to given address*/
        if (bind(server_fd, (struct sockaddr *)&s_addr, sizeof(s_addr))) {
                perror("Failed to bind server socket");
                exit(1);
        }
	printf("Bind Success, server ready\n");

	uint16_t code;

	/* Set the a handle to manage the killed child process */
	signal(SIGCHLD, handle_child);


	while (1) {
	/* Read the data on server fd */ 
	memset(buf, 0, 1000);
	read = recvfrom(server_fd, buf, 1000, 0 , (struct sockaddr *)&caddr, &clen); 
	if (read < 0) {
		perror("Error in recvfrom\n");
	}

	memcpy(&code, buf, sizeof(uint16_t));
        code = ntohs(code);
	printf("Incoming request from %s  for opcode %d \n", inet_ntoa(caddr.sin_addr), code);
	buf[strlen(buf)] = '\0';
	

	if (code == TFTP_RRQ || TFTP_WRQ) {
		/* create a child process*/
	if (!fork()) {
		
		switch(code) {
			case TFTP_RRQ:
				fd = tftp_handle_rrq(buf, caddr, clen, server_fd);
				break;
			case TFTP_WRQ:
				fd = tftp_handle_wrq(buf, caddr, clen, server_fd);
				break;
			default:
				printf("Invalid opcode\n");
		}
	

	
	printf("file send, kill client \n");
	close(client_fd);
	exit(0);
	} // fork
	} // code

	}



}
