
// This is the client code for MP2
// author: kdu1113@tamu.edu
#include<stdio.h> 
#include<stdlib.h> 
#include<sys/socket.h> 
#include<sys/types.h> 
#include<netinet/in.h>
#include<string.h> 
#include<strings.h> 
#include<errno.h> 
#include<unistd.h> 
#include<arpa/inet.h>
#include<stdbool.h>
#include "chat.h"
#include<netdb.h>
#include <signal.h>
#include "CustomStruct.h"

#define MAX_BUFF_SIZE (1200)
#define MAX_MESSAGE_LEN (512)

// Read from socket and do whatever the client needs to do
int readMessagefromServer(int sockfd){
  struct Message stCustomMessage;
  int rslt = 0;
  int nbytes=0;
  int value, i;

  nbytes=read(sockfd,(struct Message *) &stCustomMessage,sizeof(stCustomMessage));
  if(nbytes <=0){
    perror("Server Not Connected.\n");
    kill(0, SIGINT);
    exit(1);
  }

  // (3) FWD
  if(stCustomMessage.header.type==MSG_TYPE_FWD)
  {
    if((stCustomMessage.attribute[0].payload!=NULL || stCustomMessage.attribute[0].payload!='\0') &&
        (stCustomMessage.attribute[1].payload!=NULL || stCustomMessage.attribute[1].payload!='\0') && 
        stCustomMessage.attribute[0].type==ATTR_TYPE_MESSAGE && stCustomMessage.attribute[1].type==ATTR_TYPE_USER_NAME)
    {
      printf("%s : %s ", stCustomMessage.attribute[1].payload, stCustomMessage.attribute[0].payload);
    }
    return 0;
  }

  // (5) NAK
  if(stCustomMessage.header.type==MSG_TYPE_NAK)
  {
    if((stCustomMessage.attribute[0].payload!=NULL || stCustomMessage.attribute[0].payload!='\0') && 
        stCustomMessage.attribute[0].type==1)
    {
            printf("Disconnected.NAK Message from Server is %s \n",stCustomMessage.attribute[0].payload);
    }
    return 1;
  }

  // (6) OFFLINE
  if(stCustomMessage.header.type==MSG_TYPE_OFFLINE)
  {
    if((stCustomMessage.attribute[0].payload!=NULL || stCustomMessage.attribute[0].payload!='\0') && 
      stCustomMessage.attribute[0].type==2)
    {
            printf("User '%s' is now OFFLINE \n",stCustomMessage.attribute[0].payload);
    }
    return 0;
  }

  // (7) ACK
  if(stCustomMessage.header.type==MSG_TYPE_ACK)
  {    	
	  if((stCustomMessage.attribute[0].payload!=NULL || stCustomMessage.attribute[0].payload!='\0') && 
        stCustomMessage.attribute[0].type==4)
    {
                printf("ACK Message from Server is %s \n",stCustomMessage.attribute[0].payload);
    }
    return 0;
  }

  // (8) ONLINE
  if(stCustomMessage.header.type==MSG_TYPE_ONLINE)
  {
    if((stCustomMessage.attribute[0].payload!=NULL || stCustomMessage.attribute[0].payload!='\0') && 
       stCustomMessage.attribute[0].type==2)
    {
            printf("User '%s' is ONLINE \n",stCustomMessage.attribute[0].payload);
    }
    return 0;
  }

  // (9) IDLE Message
  if(stCustomMessage.header.type==MSG_TYPE_IDLE)
  {
    if((stCustomMessage.attribute[0].payload!=NULL || stCustomMessage.attribute[0].payload!='\0') && 
       stCustomMessage.attribute[0].type==2)
    {
      printf("User '%s' is in IDLE state \n",stCustomMessage.attribute[0].payload);
    }
    return 0;
  }
  
  return 0;
}

int isValidIpAddr(char* addr_str)
{
	struct sockaddr_in tmp_sock;
	int rslt = inet_pton(AF_INET, addr_str, &(tmp_sock.sin_addr));
	if (rslt == 0)
	{
		return false;
	}
	return true;
}

void send_join(int sock_fd, char* username){
  SBCP_Attribute stAttr;
  SBCP_Message stMessage;

  memset(&stMessage, 0, sizeof(stMessage));
  memset(&stAttr, 0, sizeof(stAttr));

  stAttr.type = ATTR_TYPE_USER_NAME;
  stAttr.length = strlen(username) + 1;
  strcpy(stAttr.payload, username);

  stMessage.vrsn = SBCP_PROTOCOL_VERSION;
  stMessage.type = MSG_TYPE_JOIN;
  memcpy(&(stMessage.stAttribute1), &stAttr, sizeof(stAttr));

  printf("username : %s\n", stAttr.payload);

  write(sock_fd,(void *) &stMessage,sizeof(stMessage));
  // if (writen(sock_fd, (void *) &stMessage, sizeof(stMessage)) == -1)
  // {
  //   printf("[ERROR]Writen failed.\n");
  //   exit(-1);
  // }

  // Wait for server's reply
  sleep(1);
  if(readMessagefromServer(sock_fd) == 1)
  {
    close(sock_fd);
    exit(0);
  }
}

void send_message(int sock_fd){
  SBCP_Attribute stAttr;
  SBCP_Message stMessage;

  memset(&stMessage, 0, sizeof(stMessage));
  memset(&stAttr, 0, sizeof(stAttr));

  int n = 0;
  char buff[MAX_BUFF_SIZE];
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);
  
  select(STDIN_FILENO+1, &readfds, NULL, NULL, NULL);

  if (FD_ISSET(STDIN_FILENO, &readfds))
  {
    n = read(STDIN_FILENO, buff, sizeof(buff));
    if(n > 0){
      buff[n] = '\0';
    }
  }

  stMessage.type = MSG_TYPE_SEND;
  strcpy(stAttr.payload,buff);
  stAttr.type = ATTR_TYPE_MESSAGE;
  memcpy(&(stMessage.stAttribute1), &stAttr, sizeof(stAttr));
  write(sock_fd ,(void *) &stMessage, sizeof(stMessage));
}

void send_timeout(int sock_fd){
  SBCP_Attribute stAttr;
  SBCP_Message stMessage;

  memset(&stMessage, 0, sizeof(stMessage));
  memset(&stAttr, 0, sizeof(stAttr));

  stMessage.vrsn=SBCP_PROTOCOL_VERSION;
  stMessage.type=MSG_TYPE_IDLE;
  write(sock_fd,(void *) &stMessage, sizeof(stMessage));
}

int main(int argc, char* argv[])
{
	int tcp_socket;
	struct sockaddr_in servaddr;
  printf("starting client.\n");

	// Step0. Check errors
	if (argc < 4)
	{
		printf("[Error] Usage: ./client <username> <IP address> <port>\n");
		exit(-1);
	}
	if (isValidIpAddr(argv[2]) == false)
	{
		printf("[Error] Invalid IP address.\n");
		exit(-1);
	}

	// Step1. Create a Socket
	tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_socket == -1)
	{
		printf("[Error] socket creation failed.\n");
		exit(-1);
	}
	else
	{
		// socket successfully created. Do nothing.
#if (DEBUG_PRINT == 1)
		printf("Socket created.\n");	
#endif
	}
	
	// Step2. Connect
	servaddr.sin_family = AF_INET;
  struct hostent* hostret = gethostbyname(argv[2]);
  memcpy(&servaddr.sin_addr.s_addr, hostret->h_addr,hostret->h_length);
	servaddr.sin_addr.s_addr = inet_addr(argv[2]);
	servaddr.sin_port = htons(atoi(argv[3]));
	
	if (connect(tcp_socket, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		printf("[Error] Connect Failed.\n");
		exit(-1);
	}

#if (DEBUG_PRINT == 1)
	printf("Connected to the server.");
#endif

  char* username = argv[1];

  // Send Join Command
  send_join(tcp_socket, username);
  printf("Server connection successful. \n");

  char buff[MAX_BUFF_SIZE];
  char send_buff[MAX_BUFF_SIZE];
  fd_set master;
  fd_set new;
  fd_set read_fds;

  FD_ZERO(&master);
  FD_ZERO(&new);
  FD_ZERO(&read_fds);

  FD_SET(tcp_socket, &master);
  FD_SET(STDIN_FILENO, &new);

  SBCP_Message stMessage;
  SBCP_Attribute stAttribute;
    
  struct timeval tv;
  int secs, usecs;
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  secs=(int) tv.tv_sec;
  usecs=(int) tv.tv_usec;

  pid_t pid;
  pid=fork();

  if(pid==0){
    // select stdin
    while(1){
      read_fds=new;
      tv.tv_sec = 10;
      tv.tv_usec = 0;

      if(select(STDIN_FILENO+1, &read_fds, NULL, NULL, &tv) == -1)
      {
        perror("select faiiled.");
        exit(0);
      }
      // if stdin ready
      if (FD_ISSET(STDIN_FILENO, &read_fds)){
        // create a packet and send it to the server.
        send_message(tcp_socket);
        continue;
      }
      else if(tv.tv_sec==0 && tv.tv_usec==0){
        printf("Time out!! No user input for %d secs %d usecs\n", secs, usecs);
        tv.tv_sec=10;
        tv.tv_usec=0;
        read_fds=new;
        send_timeout(tcp_socket);
        continue;
			}
    }
  }
  else {
    // master
    while(1){
      read_fds=master;
      // select server
      if (select(tcp_socket+1,&read_fds,NULL,NULL,NULL) == -1)
      {
        perror("select failed.");
        exit(1);
      }
      // if data ready from socket
      if (FD_ISSET(tcp_socket, &read_fds)){
        // read the message and do sth.
        readMessagefromServer(tcp_socket);
      }
    }
    kill(pid, SIGINT);
  }

  printf("\n Connection Ends \n");

  return 0;
}
