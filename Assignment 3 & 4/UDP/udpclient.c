/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
#include <sys/wait.h>

#define BUFSIZE 1050
#define DATASIZE 32000
#define MAX_CONN_REQ 5

/* 
 * error - wrapper for perror
 */
void error(std::string msg) {
    perror(msg.c_str());
    exit(0);
}

int writer(int sockfd, char *buf, struct sockaddr_in serveraddr,int serverlen)
{
	int n;
	n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&serveraddr, serverlen);
    if (n < 0) 
      error("ERROR in sendto");
    return n;
}

int reader(int sockfd, char *buf, struct sockaddr_in serveraddr, int serverlen)
{
	int n;
	bzero(buf,BUFSIZE);
	n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)&serveraddr, (socklen_t*)&serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);
    return n;
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    FILE *fp;
    char filename[100];
    char hello_msg[100], str[16];
    char filereader[100];
    char data[DATASIZE];
    char *dataptr = data;
    int x;
    char md5_data[121];
    int chunks;
    int sequence;
    int timeout;
    bool ack_recv;
    int myseq=0;
    int ackseq;

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
    
    /*calculate server length*/
    serverlen = sizeof(serveraddr);

    //---------------------------------open file-------------------------------------
     /* get filename from the user*/ 
    printf("Please enter file name: ");
    bzero(buf, BUFSIZE);
    scanf("%s",filename);
    
    /*read from file*/
    fp = fopen(filename,"r");
    if(fp == NULL) 
    	error("no such file");
    strcpy(data,"");
    while(fgets(filereader,100,fp))
    {
    	strcat(data,filereader);
    }
    fclose(fp);
    
    chunks = (strlen(data)/1000)+1;
    
    x = fork();
    if(x == 0)
    {
    	int fd = open("md5_value.txt", O_WRONLY | O_CREAT);
    	if(close(STDOUT_FILENO)==-1)error("error closing stdout");
    	if(dup(fd)==-1)error("error while copying descriptor");
    	if(execlp("md5sum","md5sum",filename,NULL)==-1) error("error in finding md5um");
    }
    wait(NULL);
    //------------------------------------------------send Initial hello----------------------------------------------------------------
    /*hello message*/
    strcpy(hello_msg,"hey, ");
    strcat(hello_msg,filename);
    snprintf(str,16,"%d",strlen(data));
    strcat(hello_msg," ");
    strcat(hello_msg,str);
    snprintf(str,16,"%d",chunks);
    strcat(hello_msg," ");
    strcat(hello_msg,str);
    
    /* send the message to the server */
    
    ack_recv = false;
    
    for(int i=0 ; i<MAX_CONN_REQ && !ack_recv; i++)
    {
    	 n = writer(sockfd,hello_msg,serveraddr,serverlen);

		fd_set fds ; //it's set of descriptors who want to read

		struct timeval tv ;

		// Set up the file descriptor set.
		FD_ZERO(&fds) ; //set fds = null set
		FD_SET(sockfd, &fds) ; // add sockfd as one of the descriptor in fds, i.e. sockfd wants to read

		// Set up the struct timeval for the timeout.
		tv.tv_sec = 3 ;
		tv.tv_usec = 0 ;

		// Wait until timeout or data received.
		timeout = select ( sockfd+1, &fds, NULL, NULL, &tv );

		/* print the server's reply */
		ack_recv = (FD_ISSET(sockfd, &fds)) ? (n = reader(sockfd,buf,serveraddr,serverlen)) : false ;
		
		//check for correct acknowledgement
		if(strcmp(buf,"hey")!=0) ack_recv = false;
		
		if(!ack_recv) std::cout<<"no acknowledgement recieved"<<std::endl;
    }
    if(!ack_recv) error("Couldn't connect with server\n");
    
    //---------------------------------------------send the data ---------------------------------------------------------------------------------
    
    for(int i=0 ; i<chunks; i++)
    {
    	//create message
    	char temp_buf[1025];
    	strncpy(temp_buf,dataptr,1024);
    	dataptr+=1024;
    	temp_buf[1024]='\0';
    	sprintf(str,"%d",myseq%2);
    	strcpy(buf,str);
    	strcat(buf," ");
    	sprintf(str,"%04d",strlen(temp_buf));
    	strcat(buf,str);
    	strcat(buf, " ");
    	strcat(buf,temp_buf);
    	//std::cout<<"buff : \n\n\n\n"<<buf<<"\n\n\n\n"<<std::endl;
    	
    	//send msg
    	
    	ack_recv = false;
    
		for(int i=0 ; i<MAX_CONN_REQ && !ack_recv; i++)
		{
			 n = writer(sockfd,buf,serveraddr,serverlen);
	
			fd_set fds ; //it's set of descriptors who want to read

			struct timeval tv ;

			// Set up the file descriptor set.
			FD_ZERO(&fds) ; //set fds = null set
			FD_SET(sockfd, &fds) ; // add sockfd as one of the descriptor in fds, i.e. sockfd wants to read

			// Set up the struct timeval for the timeout.
			tv.tv_sec = 3 ;
			tv.tv_usec = 0 ;

			// Wait until timeout or data received.
			timeout = select ( sockfd+1, &fds, NULL, NULL, &tv );

			
			ack_recv = (FD_ISSET(sockfd, &fds)) ? (n = reader(sockfd,buf,serveraddr,serverlen)) : false ;
		
			//check for correct seq number
			char *ptr;
			   
			ackseq = strtol(buf, &ptr, 10);
			printf("\nackseq %d and myseq %d\n",ackseq,myseq);
			if(ackseq!=(myseq%2)) ack_recv = false;
		
			if(!ack_recv) std::cout<<"no acknowledgement recieved"<<std::endl;
	
		}
		if(!ack_recv) error("Couldn't connect with server\n");
    	
    	myseq++;
    }
    
    //------------------------------------------tell that no more file and ask for md5-----------------------------------------------------------------------
    
    strcpy(buf,"over");
    
    ack_recv = false;
    
    for(int i=0 ; i<MAX_CONN_REQ && !ack_recv; i++)
    {
    	 n = writer(sockfd,buf,serveraddr,serverlen);

		fd_set fds ; //it's set of descriptors who want to read

		struct timeval tv ;

		// Set up the file descriptor set.
		FD_ZERO(&fds) ; //set fds = null set
		FD_SET(sockfd, &fds) ; // add sockfd as one of the descriptor in fds, i.e. sockfd wants to read

		// Set up the struct timeval for the timeout.
		tv.tv_sec = 3 ;
		tv.tv_usec = 0 ;

		// Wait until timeout or data received.
		timeout = select ( sockfd+1, &fds, NULL, NULL, &tv );

		/* print the server's reply */
		ack_recv = (FD_ISSET(sockfd, &fds)) ? (n = reader(sockfd,buf,serveraddr,serverlen)) : false ;
		
		if(!ack_recv) std::cout<<"no acknowledgement recieved"<<std::endl;
    }
    if(!ack_recv) error("Couldn't connect with server\n");
    
    //----------------------------------------comparing md5_value-------------------------------------------------------------------------------------
    
    //buf has md5 value returned from server
    
    fp = fopen("md5_value.txt","r");
    if(fp == NULL) 
    	error("no such file");
    fscanf(fp,"%s",md5_data);
    
    if(strcmp(md5_data,buf)==0)printf("\n\nMD5 MATCHED\n");
    else printf("\n\nMD5 NOT MATCHED\n");
    
    close(sockfd);
    
    return 0;
}
