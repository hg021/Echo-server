#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "sbcp_util.h"

#include<time.h>


//#define PORTVAL "4000"

//#define MAXBUFFERSZ 100  // size of the read and write buffer



#define IDLE_TIME 10     // in seconds

#define JOIN_IDLE_TIME 2 // in seconds for join function

// function for sending idle
void sendidle(int scktfd,char *usrname){

sbcp_msg msgstruct;

// Header for msg 
msgstruct.hdr.vrsn=3;
msgstruct.hdr.type=SB_IDLE;

msgstruct.attr[AT_USRNAME].type=AT_USRNAME;
strcpy(msgstruct.attr[AT_USRNAME].payload,usrname);


//length of arguments written arguments 
msgstruct.hdr.length=sizeof(msgstruct);
msgstruct.attr[AT_USRNAME].length=strlen(usrname)+1;

// writing idle packet into the network
if(write(scktfd,(void *)&msgstruct,sizeof(msgstruct))<=0){

perror("\nmessage struct write in send idle function failed\n");

}




}



// Reading server message - function

int readsrvmsg(int scktfd){


sbcp_msg msgstruct;
int nread=0;
int status=1;

// status 1 indicates NAK which is used to quit the program

nread=read(scktfd,(struct sbcp_msg *)&msgstruct,sizeof(msgstruct));
if(nread < 0){printf("\n No data read in readsrvmsg function \n");}

else{
// to read forwarded msg. The attibute, header types are checked 
// for conformity
if(msgstruct.hdr.type==SB_FWD){

if((msgstruct.attr[AT_MESSAGE].type==AT_MESSAGE) && (msgstruct.attr[AT_USRNAME].type==AT_USRNAME))
	{     	
		printf("%s: %s \n", msgstruct.attr[AT_USRNAME].payload,
		msgstruct.attr[AT_MESSAGE].payload);
	}
        status=0;


}

// to read NAK msg. The attibute, header types are checked 
// for conformity
if(msgstruct.hdr.type==SB_NAK){

if((msgstruct.attr[AT_REASON].payload!=NULL || msgstruct.attr[AT_REASON].payload!='\0')&& 
msgstruct.attr[AT_REASON].type==AT_REASON){

 printf("\nNAK sent from server. MSG: %s \n",msgstruct.attr[AT_REASON].payload);

}

status =1;

}

// to read OFFLINE msg. The attibute, header types are checked 
// for conformity

if(msgstruct.hdr.type==SB_OFFLINE){

if((msgstruct.attr[AT_USRNAME].payload!=NULL || 
msgstruct.attr[AT_USRNAME].payload!='\0')&& msgstruct.attr[AT_USRNAME].type==AT_USRNAME){

 printf("%s has gone offline \n",msgstruct.attr[AT_USRNAME].payload);

}

status=0;


}


// to read ACK msg. The attibute, header types are checked 
// for conformity
if(msgstruct.hdr.type==SB_ACK){

if((msgstruct.attr[AT_USRNAME].payload!=NULL || msgstruct.attr[AT_USRNAME].payload!='\0')&& 
(msgstruct.attr[AT_COUNT].payload!=NULL || msgstruct.attr[AT_COUNT].payload!='\0')
&& msgstruct.attr[AT_COUNT].type==AT_COUNT){

 printf(" ACK msg :%s\n Clients in chatroom: %s  \n",msgstruct.attr[AT_COUNT].payload,
 msgstruct.attr[AT_USRNAME].payload);

}

status=0;


}

// to read ONLINE msg. The attibute, header types are checked 
// for conformity

if(msgstruct.hdr.type==SB_ONLINE){

if((msgstruct.attr[AT_USRNAME].payload!=NULL || msgstruct.attr[AT_USRNAME].payload!='\0')&&
 msgstruct.attr[AT_USRNAME].type==AT_USRNAME){

 printf(" User %s is online : \n",msgstruct.attr[AT_USRNAME].payload);

}

status=0;



}

// to read IDLE msg. The attibute, header types are checked 
// for conformity

if(msgstruct.hdr.type==SB_IDLE){

if((msgstruct.attr[AT_USRNAME].payload!=NULL || msgstruct.attr[AT_USRNAME].payload!='\0')&&
 msgstruct.attr[AT_USRNAME].type==AT_USRNAME){

 printf(" %s is IDLE: \n",msgstruct.attr[AT_USRNAME].payload);

}

status=0;


}
return status;



}
}


// join chat function 

void joinchat(int scktfd,char *usrname){

sbcp_msg msgstruct;

struct timeval tmv_join;

int srvstatus=1; // If NAK is received, srvstatus=1 - flag to quit

int ackloopv=0;

// Initialising list for SELECT
fd_set readfdst_join,master_join;
// setting timer to JOIN_IDLE_TIME. 
// waits for JOIN_IDLE_TIME sec for ack, then quits on non arrival 
tmv_join.tv_sec =JOIN_IDLE_TIME ;
tmv_join.tv_usec =0;

FD_ZERO(&readfdst_join);
FD_ZERO(&master_join);


FD_SET(scktfd,&master_join);
readfdst_join=master_join;

// Constructing the JOIN message
msgstruct.hdr.vrsn=3;
msgstruct.hdr.type=SB_JOIN;

msgstruct.attr[AT_USRNAME].type=AT_USRNAME;
strcpy(msgstruct.attr[AT_USRNAME].payload,usrname);
printf("\n Sending join request for %s \n",msgstruct.attr[AT_USRNAME].payload);

// check the length arguments 
msgstruct.hdr.length=strlen(usrname)+1;
msgstruct.attr[AT_USRNAME].length=strlen(usrname)+1;


// sending the join message
if(write(scktfd,(void *)&msgstruct,sizeof(msgstruct))<=0){

perror("\nmessage struct write in join failed\n");
exit(1);
}

else{printf(" Sent ACK request \n");}

select(scktfd+1,&readfdst_join,NULL,NULL,&tmv_join);

// waits for ack. If ACK is received, srvstatus=0
// for NAK, srvstatus=1. used to quit the chat
if(FD_ISSET(scktfd,&readfdst_join)){

srvstatus= readsrvmsg(scktfd);}



if(srvstatus==1){
printf("\n Unable to join chat ack receive failed or\
 NAK received. Exiting from join function \n");
close(scktfd);
exit(1);
}



}




// chat function 

void chatmsg(int scktfd,char *usrname){



int numread=0;
fd_set readfdst;
struct timeval tmv;

sbcp_msg msgstruct;

// Constructing message


msgstruct.hdr.vrsn=3;
msgstruct.hdr.type=SB_SEND;

msgstruct.attr[AT_MESSAGE].type=AT_MESSAGE;

msgstruct.attr[AT_USRNAME].type=AT_USRNAME;


// Timer for timeout in chat
tmv.tv_sec =2 ;
tmv.tv_usec =0;

FD_ZERO(&readfdst);

FD_SET(STDIN_FILENO,&readfdst);

select(STDIN_FILENO+1,&readfdst,NULL,NULL,&tmv);

if(FD_ISSET(STDIN_FILENO,&readfdst)){

numread=read(STDIN_FILENO,msgstruct.attr[AT_MESSAGE].payload,sizeof(msgstruct.attr[AT_MESSAGE].payload));

strcpy(msgstruct.attr[AT_USRNAME].payload,usrname);


if(numread < 0){printf(" No data read in chat function \n");}

msgstruct.attr[AT_MESSAGE].payload[numread]='\0';


msgstruct.hdr.length=sizeof(msgstruct);

msgstruct.attr[AT_MESSAGE].length= sizeof(msgstruct.attr[AT_MESSAGE]);

// writing chat message
if(write(scktfd,(void *)&msgstruct,sizeof(msgstruct))<=0){

perror("write failure in func chatmsg \n");

}

}

else{

printf("Stdin timeout \n");

}


}







void *getinpaddr(struct sockaddr *soca){ // for getting ip address from socket structure

 if(soca->sa_family == AF_INET){
  return &(((struct sockaddr_in*)soca)->sin_addr);
 
 }

 return &(((struct sockaddr_in6 *)soca)->sin6_addr);

}


int main(int argc,char *argv[]){

int scktfd ;
struct addrinfo addrinfoval, *res_adv ;
struct timeval tmv_main;
int chk_v ;
char ipstr[INET6_ADDRSTRLEN];



int nsent,nrecv;

// Initialisations for select 
fd_set master;
fd_set read_fds;


FD_ZERO(&read_fds);
FD_ZERO(&master);


if(argc!=4){

fprintf(stderr,"use file_ex_arg usrname ipaddress portval  ");// input verification 
exit(1);

}


memset(&addrinfoval,0,sizeof(addrinfoval));
addrinfoval.ai_family=AF_INET;
addrinfoval.ai_socktype=SOCK_STREAM;

if((chk_v=getaddrinfo(argv[2],argv[3],&addrinfoval,&res_adv))!=0){ // gets information about ip in argv[2] and 
 
// argv[3] is portvalue
fprintf(stderr,"\nclient : getaddrinfo error :%s\n",gai_strerror(chk_v)); // checks for getaddrinfo error

exit(1);

}

if((scktfd=socket(res_adv->ai_family,res_adv->ai_socktype,
res_adv->ai_protocol)) == -1){ // creating a socket

perror("client : creation of socket scktfd var");
exit(1);
}

if(connect(scktfd,res_adv->ai_addr,res_adv->ai_addrlen)==-1){ // connecting socket to network
close(scktfd);
perror("client: connect func error");
exit(1);
}

inet_ntop(res_adv->ai_family,getinpaddr((struct sockaddr *)res_adv->ai_addr),
ipstr,sizeof(ipstr));  // converting the ipaddress from network byte order to host byte order

fprintf(stdout,"client: connected to %s\n",ipstr); // Printing server ip on stdout 
freeaddrinfo(res_adv);



printf("\nconnection to server has been established\n");

joinchat(scktfd,argv[1]); // sends JOIN msg to server
FD_SET(scktfd,&master); // adding socket to select list
FD_SET(STDIN_FILENO,&master); // adding STDIN_FILENO to select list




for(;;){

read_fds=master;// copying original list since select modifies list

tmv_main.tv_sec =IDLE_TIME ; // Timer for IDLE packet
tmv_main.tv_usec =0;

if(select(scktfd+1,&read_fds,NULL,NULL,&tmv_main)==-1){

perror("select error");
exit(1);
}

if(FD_ISSET(scktfd,&read_fds)){
readsrvmsg(scktfd); // reading server message

}

else if(FD_ISSET(STDIN_FILENO,&read_fds)){
printf("ME: \n");
 chatmsg(scktfd,argv[1]); // reading STDIN message
}

else{
printf("Sending IDLE message \n");
sendidle(scktfd,argv[1]);} // upon timeout, sending IDLE message


}



printf("\nExit from chat complete\n");

close(scktfd); // closing socket

return 0;

}
