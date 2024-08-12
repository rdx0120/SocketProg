
// this is the client code
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

//#define DEBUG_PRINT 1
#define CLIENT_BUFF_SIZE 10
#define SERVER_BUFF_SIZE 10

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

int writen(int file_descriptor, const void* buf, size_t nMessageLen)
{
	int nSent = 0;
	int nRemaining = nMessageLen;
	int nRet = 0;	
#if (DEBUG_PRINT == 1)
	printf("writen loop start.\n");
#endif
	while (nSent < nMessageLen)
	{
		nRet = write(file_descriptor, buf, nRemaining);
		if (nRet == -1)
		{
			if (errno == EINTR)
			{
				printf("EINTR happened during write operation.\n");
				continue;
			}
			return -1;
		}
		nSent += nRet;
		nRemaining -= nRet;
		buf += nRet;	// move pointer location.
	}
	return 0;
}

int readline(int file_descriptor, void *buf, size_t nMessageLen)
{
	// ssize_t read(int fildes, void *buf, size_t nbyte);
	int received = 0;
	int nRemaining = nMessageLen;
	int nRet = 0;
#if (DEBUG_PRINT == 1)
	printf("readline loop start.\n");
#endif
	while (received < nMessageLen)
	{
		// read one char
		nRet = read(file_descriptor, buf, 1);
		if (nRet == -1)	
		{
			if (errno = EINTR)
			{
				printf("EINTR happened during read operation.\n");
				continue;
			}
			return -1;
		}
		else if (nRet == 0 || ((char*)buf)[0] == '\n')
		{
			// EOF or new line.
			// null terminate the string.
			((char*)buf)[0] = 0;
			return received;
		}
		nRemaining--;
		received++;
		buf += 1;	
	}
	return received;
}


// Usage: ./client <IP addr> <port>
int main(int argc, char* argv[])
{
	int tcp_socket;
	struct sockaddr_in servaddr;

	// Step0. Check errors
	if (argc < 3)
	{
		printf("[Error] IP address or Port not specified.\n");
		exit(-1);
	}
	if (isValidIpAddr(argv[1]) == false)
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
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	servaddr.sin_port = htons(atoi(argv[2]));
	
	if (connect(tcp_socket, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
	{
		printf("[Error] Connect Failed.\n");
		exit(-1);
	}

#if (DEBUG_PRINT == 1)
	printf("Connected to the server.");
#endif
	
	// Step3. loop write & read
	char c_buff[CLIENT_BUFF_SIZE+1];
	char s_buff[SERVER_BUFF_SIZE+1];
	int message_length;
	int written_length;

	while (1)
	{
		// Clear buffer
		memset(&c_buff, 0, sizeof(c_buff));
		memset(&s_buff, 0, sizeof(s_buff));		
		
		// Step 3-1. read from stdin (When EOF is in stdin, break from the loop)
		printf("Enter the string: ");
		if (feof(stdin) == true || fgets(c_buff, CLIENT_BUFF_SIZE, stdin) == NULL)
		{
			printf("\nEOF detected.\n");
			break;		
		}
					
		// Step 3-2. write line to server.
		printf("Client Sending: %s\n", c_buff);
		message_length = strlen(c_buff) + 1;
#if (DEBUG_PRINT == 1)
		// printf("length: %d\n", message_length);
#endif	
		if (writen(tcp_socket, c_buff, message_length) == -1)
		{
			printf("[ERROR]Writen failed.\n");
			exit(-1);
		}
#if (DEBUG_PRINT == 1)
		printf("writen function finished.\n");
#endif

		// Step 3-3. read echoed line from the network and print to stdout. 
		int read_length = readline(tcp_socket, s_buff, SERVER_BUFF_SIZE);
		if (read_length == -1)
		{
			printf("[ERROR] readline failed.\n");
			exit(-1);
		}
#if (DEBUG_PRINT == 1)
		printf("readline function finished.\n");
#endif
		printf("Recieved from server: ");
		printf("%s\n", s_buff);	
	}		
	
	// Step 4. Close connection.
	printf ("EOF detected from stdin. Closing connection.\n");
	close(tcp_socket);	
	
	return 0;
}
