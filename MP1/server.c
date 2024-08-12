//This is the server code
// author: rohan.dalvi@tamu.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h> // Include errno for error handling

#define DEBUG_PRINT 1
#define SERVER_BUFF_SIZE 10

// Function to write data to a file descriptor.
int server_writen(int file_descriptor, const void *buf, size_t nMessageLen)
{
    ssize_t nSent = 0;
    ssize_t nRemaining = nMessageLen;
    ssize_t nRet = 0;    

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
        buf += nRet;    // Move the pointer location.
    }
    return 0;
}

// Function to handle communication with a client.
void server_handle_client(int server_client_socket) {
    char server_buffer[SERVER_BUFF_SIZE+1];
    ssize_t server_bytes_received;

    while (1) {
        // Read data from the client.
        server_bytes_received = read(server_client_socket, server_buffer, sizeof(server_buffer));
        if (server_bytes_received <= 0) {
            // Connection closed by client or an error.
            break;
        }
        
        printf("Received from client: %s\n", server_buffer);
        
	// Echo > received data > client
        server_writen(server_client_socket, server_buffer, server_bytes_received);

        // Clearing buffer.
        memset(&server_buffer, 0, sizeof(server_buffer));
    }

    // Client socket closed & child process exit
    printf("Client disconnected.\n");
    close(server_client_socket);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
#if (DEBUG_PRINT == 1)
    printf("argument check passed.\n");
#endif

    int server_port = atoi(argv[1]);
    int server_socket, server_client_socket;
    struct sockaddr_in server_address, server_client_address;
    socklen_t server_client_length = sizeof(server_client_address);

    // Socket creation for the server.
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket_Creatn Failed");
        exit(1);
    }
#if (DEBUG_PRINT == 1)
    printf("Socket created.\n");
#endif
    // Socket binding to the server address.
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Socket_Bind Failed");
        close(server_socket);
        exit(1);
    }
#if (DEBUG_PRINT == 1)
    printf("Socket binded.\n");
#endif

    // Listening for any new connection
    if (listen(server_socket, 5) == -1) {
        perror("Socket_Listn Failed");
        close(server_socket);
        exit(1);
    }
    printf("Server happens to listen on port %d...\n", server_port);

    while (1) {
        // Accepting new client connection.
        server_client_socket = accept(server_socket, (struct sockaddr *)&server_client_address, &server_client_length);
#if (DEBUG_PRINT == 1)
        printf("accept function called.\n");
#endif
        if (server_client_socket == -1) {
            perror("Client_Connectn Failed");
            continue;
        }

	// [kdu1113@tamu.edu] this printf causes Segment fault.
        // printf("Accepted_Connection from %s:%d\n", inet_ntoa(server_client_address.sin_addr.s_addr), ntohs(server_client_address.sin_port));

        // Handling client communication in a different process.
        int pid = fork();
	if (pid == 0) {
            close(server_socket); 
            server_handle_client(server_client_socket);
        }
        else if (pid == -1){
            // error case
            perror("fork error.");
            exit(1);
	}
	else {
            // parent
            printf("Child process created with PID: %d\n", pid);
	}

        close(server_client_socket); 
    }
    // Close the listening socket.
    close(server_socket);
    return 0;  
}

  
