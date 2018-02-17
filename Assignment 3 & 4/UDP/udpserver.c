/* 
 * udpserver.c - A UDP echo server 
 * usage: udpserver <port_for_server>
 */
#include "md5/md5.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
using namespace std;
#define BUFSIZE 1050
#define DATASIZE 32000
#define OFFSET 7

/*
 * error - wrapper for perror
 */
void error(std::string msg) {
  perror(msg.c_str());
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
  int portno; /* port to listen on */
  unsigned int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  char data[DATASIZE];
  strcpy(data,"");
  char str[10];
  int myseq=0;
  int recvseq;

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port_for_server>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

		/*
		 * recvfrom: receive a UDP datagram from a client
		 */

		bzero(buf, BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			 (struct sockaddr *) &clientaddr, &clientlen);
		if (n < 0)
		  error("ERROR in recvfrom");
		

		/* 
		 * gethostbyaddr: determine who sent the datagram
		 */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
				  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
		  error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
		  error("ERROR on inet_ntoa\n");
		printf("server received datagram from %s (%s)\n", 
		   hostp->h_name, hostaddrp);
		printf("server received %d/%d bytes ", strlen(buf), n);
		
		 
		 //----------------check what kind of message----------------------------------------------------------------
		
		if(!sscanf(buf,"%s",str)) error("error reading from string");
		if(strcmp(str,"hey,")==0)
		{
			strcpy(buf,"hey");
		}
		else if(strcmp(str,"over")==0)
		{
			strcpy(buf,md5(data).c_str());
			strcpy(data,"");
		}
		else
		{
			char *ptr;
			   
			recvseq = strtol(str, &ptr, 10);
			std::cout<<"recieved sequence number is : "<<recvseq<<" and myseq is"<<myseq<<endl;
			
			if(recvseq == (myseq%2))
			{
				myseq++;
				strcat(data,(buf+7)); 
				
			}
			strcpy(buf,str);
		}
		//-------------------------return appropriate message-------------------------------------------------------
		
		  /* 
		 * sendto: echo the input back to the client 
		 */
		 
		n = sendto(sockfd, buf, strlen(buf), 0, 
			   (struct sockaddr *) &clientaddr, clientlen);
		if (n < 0) 
		  error("ERROR in sendto");
		 else printf("server send %d/%d bytes: %s\n", strlen(buf), n, buf);
  }
}
