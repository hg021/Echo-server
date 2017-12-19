/* sbcp_server.c
 * ----------------------------------------------------
 * AUTHOR: HIMANSHU GUPTA
 * himgupt2@gmail.com
 *
 * ECEN 602 Fall 17
 * Simple broadcast chat server
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
#include "sbcp_util.h"

#define SBCP_VERSION 3 		/* protocol version 3*/

int client_count = 0;			/* number of clients */
sbcp_client *client_info;

fd_set master;			/* Declaring a global master set*/
fd_set read_fd;

/*
 * Verifies if the client has a valid user ID
 */
int sbcp_valid_client_id(char *id) {
	int ret = 0;
	
	for (int i = 0; i< client_count; i++) {
		if (strcmp(id , client_info[i].usrname) == 0) {
			return 1;
		}
	}
	return ret;
}

/*
 * Sends the ack message back to client
 * Input: client FD
 */

void sbcp_send_ack (int fd) {

	if (fd < 0) {
		printf("Invalid FD: send_ack failed\n");
		return;
	}

	int n;
	sbcp_hdr hdr;
	sbcp_attr usrname;
	sbcp_attr count;
	
	sbcp_msg msg;
	
	hdr.vrsn = SBCP_VERSION;
	hdr.type = SB_ACK;

	usrname.type = AT_USRNAME;
	usrname.payload[0] = '\0';
	for (int i = 0; i< client_count - 1; i++) {
		strcat(usrname.payload, client_info[i].usrname);
		strcat(usrname.payload, ", ");
	}
	strcat(usrname.payload, "\0");
	usrname.length = strlen(usrname.payload) + 1;
	
	count.type = AT_COUNT;
	sprintf(count.payload, "Client count %d", client_count);
	count.length = strlen(count.payload) + 1;

	msg.hdr = hdr;
	msg.attr[AT_USRNAME] = usrname;
	msg.attr[AT_COUNT]   = count;

	n = write(fd,(void *) &msg, sizeof(msg));
	printf("Sent ack to fd %d \n", fd);

	return;
}


/*
 * Sends a NAK message to client 
 * if the user ID is not valid.
 */

void sbcp_send_nak(int fd, int max_client) {
	if (fd<0) {
		printf("Invalid FD: send_nak failed\n");
	}

	sbcp_msg msg;

	msg.hdr.vrsn = SBCP_VERSION;
	msg.hdr.type = SB_NAK;

	msg.attr[AT_REASON].type = AT_REASON;
	if (client_count == max_client) {
		sprintf(msg.attr[AT_REASON].payload, "Chat room full, try later!");
	} else {
		sprintf(msg.attr[AT_REASON].payload, "Username already in use, please select new ID");
	}

	write(fd, (void *)&msg, sizeof(msg));
	printf("Sent NAK message to fd %d\n", fd);
	
	return;	
}

/*
 * Sends an OFFLINE message to all clients 
 * when the particular client disconnects
 */

void sbcp_send_offline (int server_fd, int client_fd, int max_fd) {

	if (server_fd<0 || client_fd<0) {

                printf("Invalid FD, can't send offline message\n");
                return;
        }

        sbcp_msg msg;

	msg.hdr.vrsn = SBCP_VERSION;
	msg.hdr.type = SB_OFFLINE;

	msg.attr[AT_USRNAME].type = AT_USRNAME;

	for (int i = 0; i <= client_count; i++) {
		if(client_info[i].connfd == client_fd) {
			strcpy(msg.attr[AT_USRNAME].payload, client_info[i].usrname);
			printf("Sending offline message for %s\n", client_info[i].usrname);
			break;
		}
	}

	for (int i = 0; i <= max_fd ; i++) {

		if(FD_ISSET(i, &master)) {

			if(i == server_fd || i == client_fd)
				continue; /* Skip the server and this client */

			if (write(i , (void *)&msg, sizeof(msg)) == -1) {
				perror("Error in sending offline message\n");
			}
		}
	}

}

/*
 * Sends and online message to all the clients 
 * except the client just accepted
 */

void sbcp_send_online (int server_fd, int client_fd, int max_fd) {

	if (server_fd<0 || client_fd<0) {

		printf("Invalid FD, can't send online message\n");
		return;
	}

	sbcp_msg msg;

	msg.hdr.vrsn = SBCP_VERSION;
	msg.hdr.type = SB_ONLINE;

	msg.attr[AT_USRNAME].type = AT_USRNAME;
	strcpy( msg.attr[AT_USRNAME].payload, client_info[client_count - 1].usrname);
	
	
	// loop through and send online to everyone
	for (int i = 0; i <= max_fd; i++) {
        	if (FD_ISSET(i, &master)) {
                	if (i == server_fd || i == client_fd)
				continue; /* skip the server and current client*/

			/* Send the online message to rest*/
                        if (write(i, (void *)&msg, sizeof(msg)) == -1) {
				perror("Unable to send online msg\n");
			}
		}
	}
	
}

/*
 * Sends an IDLE message when client is not 
 * sending message for a particular time
 */

void  sbcp_send_idle (int server_fd, int client_fd, int max_fd) {

	if (server_fd < 0 || client_fd < 0 || max_fd < 0) {
		printf("Invalid FD, can't send IDLE message\n");
		return;
	}

	sbcp_msg msg;

	msg.hdr.vrsn = SBCP_VERSION;
	msg.hdr.type = SB_IDLE;

	msg.attr[AT_USRNAME].type = AT_USRNAME;

	for (int i = 0; i < client_count; i++) {
		if (client_info[i].connfd==client_fd)
			strcpy(msg.attr[AT_USRNAME].payload, client_info[i].usrname);
	}

 	for (int i = 0; i <= max_fd ; i++) {

                if(FD_ISSET(i, &master)) {

                        if(i == server_fd || i == client_fd)
                                continue; /* Skip the server and this client */

                        if (write(i , (void *)&msg, sizeof(msg)) == -1) {
                                perror("Error in sending IDLE message\n");
                        }
                }
        }	
	
	return;
}

/*
 * Validate the JOIN message sent by client
 * Send ACK if success, NAK otherwise
 */

int sbcp_validate_client (int fd, int max_client) {
	
	if (fd < 0) {
		printf("Invalid FD: cannot validate client\n");
		return 1;
	}

	sbcp_msg msg;
	sbcp_attr usrname;
	int n;
	
	n = read(fd , (sbcp_msg *) &msg, sizeof(msg));
	usrname = msg.attr[AT_USRNAME];

	if (client_count == max_client) {
		sbcp_send_nak(fd, max_client);
		return 1;
	}
	
	if (sbcp_valid_client_id(usrname.payload)) {
		printf("Invalid user ID\n");
		sbcp_send_nak(fd, max_client);
		return 1;
	} else {
		strcpy(client_info[client_count].usrname, usrname.payload);
		
		client_info[client_count].connfd = fd;
		client_info[client_count].count = client_count;
		client_count++;
		sbcp_send_ack(fd);
		printf("User %s added, total users %d\n", usrname.payload, client_count);
	}
	return 0;
}

/*
 * Helper function to delete a client
 * which left the chatroom
 */

void sbcp_delete_client(int client_fd) {

	if (client_fd < 0) {
		printf("Invalid FD , can't close %d\n", client_fd);
		return;
	}
	int remove;

	close(client_fd);
	FD_CLR(client_fd, &master); /* Remove the client FD from the master set*/
	
	for(int i = 0; i< client_count; i++){
		if(client_info[i].connfd == client_fd) {
			remove = i;
			break;
		}
	}

	for(int i = remove; i < client_count - 1; i++) {
		client_info[i] = client_info[i+1];
	}

	/* Decrement the total client count*/
	client_count--;

	return;
}

/*
 * Helper function to receive an incoming message from
 * client. Forward accordingly.
 */

void sbcp_receive_msg(int server_fd, int client_fd, int max_fd) {

	int bytes_read;
	sbcp_msg msg, forward;
	sbcp_attr client_data, client_usrname; 

	bytes_read = read(client_fd, (sbcp_msg *)&msg, sizeof(msg));
	if (bytes_read > 0) {
		
		/*
		 * Received a message from client_fd 
		 * Dicard the message if length is not matched with 
		 * bytes read
		 */
		if (msg.hdr.length != bytes_read) {
			printf("length = %d, read = %d", msg.hdr.length, bytes_read);
			printf("Incomplete message, discarding\n");
			return;
		}
		
		if (msg.hdr.type == SB_SEND) {
			/* Update the header before forwarding the message*/
			msg.hdr.type = SB_FWD;

		printf("Forwarding msg from %s\n", msg.attr[AT_USRNAME].payload);
		for(int i=0 ; i<= max_fd; i++){
			if (FD_ISSET(i , &master)) {
				if(i == server_fd || i == client_fd)
					continue;

				if(write(i, (void *)&msg, bytes_read) == -1) {
					perror("Unable to forward\n");
				}
			}
		}

		} else if (msg.hdr.type == SB_IDLE) {

			/*Send a idle message to all the clients except this*/
			printf("User %s is IDLE\n", msg.attr[AT_USRNAME].payload);
			sbcp_send_idle(server_fd, client_fd, max_fd);
		} else {
                        /* Invalid message type, discard it*/
                        printf("Invalid message from client %d\n", client_fd);
                }

	} else {

		/* Error on read*/
		if (bytes_read == 0) {
			printf("Client %d disconnected \n", client_fd);
			/* we can send an offline message to rest of the clients */
			sbcp_send_offline(server_fd, client_fd, max_fd);
		} else {
			/* Read returned error*/
			perror("Error on read oeration");
		}
		/*Done with this client, can delete*/
		sbcp_delete_client(client_fd);	

	}

}

int main (int argc, char **argv) {

	int max_fd, ret;

	int port, max_client, size, tmp, status;
	struct sockaddr_in s_addr, c_addr;
	int server_fd, client_fd;
	struct hostent *server_ip;
	int i;


	if (argc!=4){
                printf("please specify in following format:./server <IP> <port> <max_client>");
                exit(1);
        }
	
	port = htons(atoi(argv[2]));

	max_client = atoi(argv[3]);
	client_info = (sbcp_client *) malloc (max_client*sizeof(sbcp_client));

	

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("Failed to open server socket");
		exit(1);
	}
	printf("Socket created \n");

	/* Add socket reuse options to avoid error while starting*/
        int options = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof (options));
	setsockopt(server_fd, SOL_SOCKET, SO_RCVLOWAT, &options, sizeof(options));

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

	if (listen(server_fd, max_client)) {    /* starts the server listener*/
                perror("failed to start listener");
		exit(1);
        } else {
                printf("Started the listener on port %d\n", ntohs(port));
        }

	/*
	 * Set the master and read fd in SELECT
	 * Zeroize before setting
	 */
	FD_ZERO(&master);
	FD_ZERO(&read_fd);
	FD_SET(server_fd, &master);
	max_fd = server_fd;



	while(1) {
		
		read_fd = master;

		/* Set the select with 1 extra fd than max */
		ret = select(max_fd+1, &read_fd, NULL, NULL, NULL);

		if (ret == -1) {
			perror("Failed to set select");
			exit(1);
		}

	for (i=0; i <= max_fd; i++) {		/* Loop for checking all FD*/

		if (FD_ISSET(i, &read_fd)){
			if (i == server_fd) {  /* Server fd for new connections*/

			size = sizeof(c_addr);
			client_fd = accept(server_fd, (struct sockaddr *)&c_addr, (socklen_t *)&size);
			if (client_fd < 0) {
				perror("Error accepting clients\n");
			}
			//printf("accept success for client %d \n", client_fd);

			tmp = max_fd;

			FD_SET(client_fd, &master);

			if (client_fd > max_fd) {
				max_fd = client_fd;
			}

			/* This validates the first message sent by client and sets the username */
			status = sbcp_validate_client(client_fd, max_client);
			if (status) {
				/* Invalid user*/
				printf("Client USER ID not valid\n");
				max_fd = tmp;
				/* Clear the client fd from the master set as it is not valid*/
				FD_CLR(client_fd, &master);
			} else {
				/* Valid user, send an online message to all client except this */
				printf("Client %d accepted\n", client_fd);
				sbcp_send_online(server_fd, client_fd, max_fd);
			}
			} else {
			
				/* Client connections, receive all incoming message*/
				sbcp_receive_msg(server_fd, i, max_fd);
			
			}
		}
	}
	}
	close(server_fd);
	return 0;
	

}
