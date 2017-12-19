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

//#define PORTVAL "4000"

#define MAXBUFFERSZ 100  // size of the read and write buffer




int writeline(int scktfd,char * dbufs,int dbufssz){ // function to read from stdin and send via socket

int nsent;

fprintf(stdout,"\n enter the string \n");

if ((fgets(dbufs,dbufssz,stdin)) == NULL) { //read from stdin and store in buffer
	close(scktfd);
	exit(1);
} else {
	dbufs[MAXBUFFERSZ]='\0'; 
	if((nsent=(send(scktfd,dbufs,strlen(dbufs),0)))==-1){
		perror("client : send err ");
		close(scktfd);
		exit(1);
	}
}


return nsent;

}


int readline(int scktfd,char * dbufr,int dbufrsz){ // function to receive data from server

// receiving data
int nrecv;

if((nrecv=recv(scktfd,dbufr,dbufrsz-1,0))==-1){ // receiving data in buffer
 
perror("client : recv error");
exit(1);
}

dbufr[nrecv]='\0';
fprintf(stdout,"\nclient recv str = %s\n",dbufr);

return nrecv;

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
int chk_v ;
char ipstr[INET6_ADDRSTRLEN];
char dbufs[MAXBUFFERSZ];
char dbufr[MAXBUFFERSZ];

//int portval;
// for send, recv
int nsent,nrecv;

if(argc!=3){

fprintf(stderr,"use file_ex_arg ipaddress portval");// input verification 
exit(1);

}

//portval=atoi(argv[2]);

memset(&addrinfoval,0,sizeof(addrinfoval));
addrinfoval.ai_family=AF_INET6;
addrinfoval.ai_socktype=SOCK_STREAM;

if((chk_v=getaddrinfo(argv[1],argv[2],&addrinfoval,&res_adv))!=0){ // gets information about ip in argv[1] and 
 

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

fprintf(stdout,"\nclient: connected to %s\n",ipstr); // Printing server ip on stdout 
freeaddrinfo(res_adv);


// SENDING AND RECEIVING DATA - ECHO 



while(1){ // repeatedly echoing 

//SENDING DATA

writeline(scktfd,dbufs,sizeof(dbufs)); // to write data from stdin to the socket

// receiving data

readline(scktfd,dbufr,sizeof(dbufr)); // to read data from server - echoing 

}

close(scktfd); // closing socket

return 0;

}
