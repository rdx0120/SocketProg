#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

// Constants
#define HTTP_PORT_NUMBER 80
#define BUFFER_SIZE 10000
#define MAX_CACHE_ENTRIES 10
#define MAX_NAME_LENGTH 256
#define MAX_TIME_LENGTH 50
#define MAX_FILE_NAME_LENGTH 10

// Data Structures
typedef struct {
    char url[MAX_NAME_LENGTH];
    char lastModifiedTime[MAX_TIME_LENGTH];
    char expiry[MAX_TIME_LENGTH];
    char filename[MAX_FILE_NAME_LENGTH];
    int isFilled;
} CacheEntry;

// Global Variables
CacheEntry cacheTable[MAX_CACHE_ENTRIES];

extern char *dayNames[7];
extern char *monthNames[12];

pthread_mutex_t fileLocks[MAX_CACHE_ENTRIES];

// Function Declarations
int createSocket(bool isIPv4);
void setServerAddress(struct sockaddr_in *serverAddress, char *ip, int port);
void setServerAddressIPv6(struct sockaddr_in6 *serverAddress, char *ip, int port);
void bindServer(int socketFD, struct sockaddr_in serverAddress);
void bindServerIPv6(int socketFD, struct sockaddr_in6 serverAddress);
void startListening(int socketFD);
int acceptConnection(struct sockaddr_in *clientAddresses, int clientCount, int socketFD);
int getMode(char *modeChecker);
void zombieHandlerFunc(int signum);
int formatReadRequest(char *request, char *host, int *port, char *url, char *name);
int checkCacheEntryHit(char *url);
int monthConverter(char *month);
int checkCacheEntryExpire(char *url, struct tm *currentTime);
int timeComparisonFunc(char *oldTime, char *newTime);
void sendErrorMessage(int status, int socketFD);
int handleClientMessage(int clientFD);
int checkCacheEntry(char *url);

// Main Function
int main(int argc, char *argv[]) {
    // Variable Declarations
    char buffer[BUFFER_SIZE];
    unsigned int portNumber;
    struct sockaddr_in currentAddressIPv4, remoteAddressIPv4;
    int clientFD;
    char request[BUFFER_SIZE];
    char *hostname;
    char *filePath;
    char *filePathCopy;
    char *fileIterator;
    int fileCount = 0, flag = 0;

    // Command Line Argument Validation
    if (argc < 4) {
        printf("Usage: ./client <server_ip> <port> <url>");
        exit(1);
    }

    // Initialization
    portNumber = atoi(argv[2]);
    bzero(&currentAddressIPv4, sizeof(currentAddressIPv4));
    bzero(&remoteAddressIPv4, sizeof(remoteAddressIPv4));

    // Setting up Address Structures
    currentAddressIPv4.sin_family = AF_INET;
    currentAddressIPv4.sin_addr.s_addr = INADDR_ANY;

    remoteAddressIPv4.sin_family = AF_INET;
    remoteAddressIPv4.sin_port = htons(portNumber);

    // Converting IP Address
    if (inet_aton(argv[1], (struct in_addr *)&remoteAddressIPv4.sin_addr.s_addr) == 0) {
        perror(argv[1]);
        exit(-1);
    }

    // Creating Socket
    clientFD = socket(AF_INET, SOCK_STREAM, 0);

    if (clientFD < 0) {
        perror("Error creating socket:");
        exit(-1);
    }

    // Connecting to Server
    if (connect(clientFD, (struct sockaddr *)(&remoteAddressIPv4), sizeof(struct sockaddr)) < 0) {
        perror("Connection Error:");
        exit(-1);
    }

    // Building HTTP Request
    memset(request, 0, BUFFER_SIZE);
    hostname = strtok(argv[3], "/");
    filePath = strtok(NULL, "");
    sprintf(request, "GET /%s HTTP/1.0\r\nHost:%s\r\n\r\n", filePath, hostname);
    fileIterator = filePath;

    // Parsing File Path
    while (*fileIterator != '\0') {
        if (*fileIterator == '/') {
            fileCount++;
        }
        fileIterator++;
    }

    filePathCopy = filePath;

    if (fileCount > 0) {
        while (fileCount >= 0) {
            if (flag == 0) {
                strtok(filePathCopy, "/");
                flag = 1;
            } else {
                fileIterator = strtok(NULL, "/");
            }
            fileCount--;
        }
    } else {
        fileIterator = filePathCopy;
    }

    // Sending Request to Server
    if (send(clientFD, request, strlen(request), 0) == -1) {
        perror("Send Error:");
        exit(-1);
    }

    // Handling Server Response
    FILE *filePtr;
    filePtr = fopen(fileIterator, "w");
    int recvMsgLength;
    memset(buffer, 0, BUFFER_SIZE);

    if ((recvMsgLength = recv(clientFD, buffer, BUFFER_SIZE, 0)) <= 0) {
        perror("Receive Error:");
    } else if (*(buffer + 9) == '4' && *(buffer + 10) == '0' && *(buffer + 11) == '4') {
        printf("%s", buffer);
        remove(fileIterator);
    } else {
        char *temp = strstr(buffer, "\r\n\r\n");
        fwrite(temp + 4, 1, strlen(temp) - 4, filePtr);
        memset(buffer, 0, BUFFER_SIZE);

        while ((recvMsgLength = recv(clientFD, buffer, BUFFER_SIZE, 0)) > 0) {
            fwrite(buffer, 1, recvMsgLength, filePtr);
            memset(buffer, 0, BUFFER_SIZE);
        }
    }

    // Cleanup
    fclose(filePtr);
    close(clientFD);
    return 0;
}
