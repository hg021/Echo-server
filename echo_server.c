/*
 * AUTHOR: HIMANSHU GUPTA
 * himgupt2@gmail.com
 *
 * Echo server
 */

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netdb.h>
#include<errno.h>

#define BUFFER 100

/*
 * Function to handle killed child process
 */
void handle_child(int signal) {
	pid_t pid;
	int child;
	pid = wait(&child);
	fprintf(stdout, "child %d is now terminated\n", pid);
	return;
}

/*
 * Input
 * 	readfd	read socket file descriptor
 *	buffer	buffer to store read data
 *	size	amount of bytes to read
 * Returns
 * 	ret	Number of bytes read 
 *	-1	Error
 *	0 	EOF
 */
int readline (int readfd, char *buffer, int size) {
	int ret;
	
	ret = read(readfd, buffer, size);
	if (ret > 0){
		buffer[ret] = '\0';
		return ret;
	} else if (ret < 0){
		fprintf(stderr,"Error: %d \n",errno);
		perror("Error on readfd");
		return ret;
	} else {
		perror("EOF, closing socket");
		return ret;
	}
}

/*
 * Input
 * 	writefd	write socket file descriptor
 *	buffer	buffer to read data from
 * 	length	length of bytes to be written on network
 * Returns
 * 	ret	Number of bytes written
 * 	-1	Error
 */
int writen(int writefd, char *buffer, int length) {
	int ret;
	
	ret = write(writefd, buffer, length);
	return ret;
}

int main (int argc, char **argv){
	int listenfd, connfd; 
	pid_t child_pid;
	
	int port;
	int ret, options, n, read;

	struct addrinfo info, *server;
	struct sockaddr_in client_addr;

	char buffer[BUFFER];
	
	if (argc!=2){
		fprintf(stderr,"please specify server port\n");
		exit(1);
	}
	memset(&info,0, sizeof(struct addrinfo));


	/* Structure to use for server */
	info.ai_family = AF_INET6; /*Using IPv6 */
	info.ai_socktype = SOCK_STREAM; /*TCP*/
	info.ai_flags = 0;
	info.ai_protocol = 0;

	ret = getaddrinfo(NULL, argv[1], &info, &server);
	if (ret != 0){
		fprintf(stderr, "getaddrinfo : %s\n ", gai_strerror(ret));
		exit(0);
	}
	
	port = atoi(argv[1]); /* Extract the port number from input*/

	listenfd = socket (server->ai_family, SOCK_STREAM, 0); /* Creating the TCP socket*/
	if (listenfd < 0) {
		perror("Failed to open socket");
	}

	signal(SIGCHLD, handle_child); /* Add signal handler to handle child*/

	/* Add socket reuse options to avoid error while starting*/
	options = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof (options));

	/* Bind the socket to given address*/
	if (bind(listenfd, server->ai_addr, server->ai_addrlen)) {
		perror("Failed to bind socket");
	}	

	if (listen(listenfd, 128)) { 	/* This start the server listener*/
		perror("failed to start listener");
	} else {
		fprintf(stdout,"Started the listener on port %d\n", port);
	}
	
	while (1){	/* Initiate the loop to accept and echo data*/

		int length = sizeof(client_addr);
		connfd = accept(listenfd, (struct sockaddr *)&client_addr, &length);
		if (connfd < 0) {
			perror("Error accepting client");
		}

		if ((child_pid = fork()) == 0) {
			printf("\nConnected new child with PID %d parent ID %d\n", getpid(), getppid());
		
		close(listenfd); 	/* We close the listener on the child process*/
		memset(buffer, '0', sizeof(buffer));
		/*
		 * We read on the connection till we get an EOF or error
		 * on the socket FD. 
		 */
		while( (read = readline(connfd, buffer, BUFFER)) > 0) { 

			fflush(stdout);
			fprintf(stdout, "Child[%d]: %s\n",getpid(), buffer);

      			int err = writen(connfd, buffer, read);		/* Echo back*/
      			if (err < 0) {
				perror("Client write failed\n");
				close(connfd);
				break;
			}
			buffer[0] = '\0'; /* Reset the buffer to 0*/
			
		}
		close(connfd);		/* Close the socket on child process*/
		exit(0);		/* Kill the child process*/
		}
	}
	close(connfd);

	return 0;
}
