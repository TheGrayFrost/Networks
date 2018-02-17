/* 
 * tcpclient.c - A simple TCP client
 * usage: tcpclient <host> <port>
 */
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

#define BUFSIZE 32000
/*
/* error - wrapper for perror
*/
void error(char *msg) {
    perror(msg);
    exit(0);
}

int writer(int sockfd, char * buf)
{
	int n;
	n = write(sockfd, buf, strlen(buf));
    if (n < 0) 
      error("ERROR writing to socket");
	return n;
}

int reader(int sockfd, char * buf)
{
	int n;
	bzero(buf, BUFSIZE);
    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("Echo from server: %s\n", buf);
    return n;
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    FILE *fp;
    char filename[100];
    char hello_msg[100], str[16];
    char filereader[100];
    char data[BUFSIZE];
    int x;
    char md5_data[121];
    
    //-------------------make connection------------------------------------------------------------------------------

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

    /* connect: create a connection with the server */
    if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
      error("ERROR connecting");
      
      //--------------------------file data is gathered and md5sum is calculated using command md5sum-------------------------------------------------------------
     
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
    
    x = fork();
    if(x == 0)
    {
    	int fd = open("md5_value.txt", O_WRONLY | O_CREAT);
    	if(close(STDOUT_FILENO)==-1)error("error closing stdout");
    	if(dup(fd)==-1)error("error while copying descriptor");
    	if(execlp("md5sum","md5sum",filename,NULL)==-1) error("error in finding md5um");
    }
    wait(NULL);
    
    //-------------------------------initial hello measage send-----------------------------------------------------------------------------------------------------
    
    /*hello message*/
    strcpy(hello_msg,"hey, ");
    strcat(hello_msg,filename);
    snprintf(str,16,"%d",strlen(data));
    strcat(hello_msg," ");
    strcat(hello_msg,str);
    
    /*send hello message*/ 
	n = writer(sockfd, hello_msg);

    /* print the server's reply */
    n = reader(sockfd, buf);
    
    if(strcmp(buf,"hey")!=0) error("not the expected response");
    
    //-----------------------------------------text data is sent and md5 response is returned---------------------------------------------------------------------------
    
    /* send the file text to the server*/ 
    strcpy(buf,data);
    n = writer(sockfd, buf);

    /* print the server's reply*/
    n = reader(sockfd, buf); 
    
    //----------------------------------------comparing md5_value-------------------------------------------------------------------------------------
    
    //buf has md5 value returned from server
    
    fp = fopen("md5_value.txt","r");
    if(fp == NULL) 
    	error("no such file");
    fscanf(fp,"%s",md5_data);
    
    if(strcmp(md5_data,buf)==0)printf("MD5 MATCHED\n");
    else printf("MD5 NOT MATCHED");
    
    close(sockfd);
    return 0;
}
