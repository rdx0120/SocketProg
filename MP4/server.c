#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>


#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
//#include <wait.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <pthread.h>
#include <math.h>
#include "cache_file.h"

#define BUFFER_SIZE (1 << 16)   // 64K

int read_request(char *buff, char *file_name, char *Url, char *host, int *port)
{
    char request_type[100]; // get
    char protocol[100]; // http
    char *pcUri;
    int  sscanfRet;
    sscanfRet = sscanf(buff, "%s %s %s %s", request_type, file_name, protocol, Url);

    if(strcmp(request_type, "GET")!=0)
    {
        return -1;
    }
    if(strcmp(protocol, "HTTP/1.0")!=0)
    {
        return -1;
    }

    pcUri = Url;
    pcUri = pcUri + 5;
    strcpy (Url, pcUri);
    strcpy (host, Url);
    strcat (Url, file_name);

    *port = 80;

    if(sscanfRet < 4)
    {
        return -1;
    }
    return 0;
}

int send_message_to_client(int err_number, int clientFd)
{
    char msg_400[100] = "HTTP 400 Bad Request.\n";
    char msg_404[100] = "HTTP 404 Not Found.\n";

    if (err_number == 400)
    {
        send(clientFd, msg_400, strlen(msg_400), 0);
        return 1;
    }
    else if (err_number == 404)
    {
        send(clientFd, msg_404, strlen(msg_404), 0);
        return 1;
    }

    perror("ERROR: HTTP unknown error number.");
    return -1;
}


int main(int argc, char *argv[]) {

    struct sockaddr_in serverAddress;
    struct sockaddr_storage remoteAddr;
    int serverFd;
    int proxyFd;
    int maxFd;
    int portAddr;
    fd_set fdSet;
    socklen_t socklen;

    if (argc != 3) {
        fprintf(stderr, "Please use the correct format: %s <server> <host> <port>\n", argv[0]);
        exit(0);
    }

    portAddr = atoi(argv[2]);
    serverAddress.sin_port = htons(portAddr);
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddress.sin_family = AF_INET;

    // 1. Create socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("ERROR: Couldn't create a socket");
        exit(0);
    } else
        printf("Server socket is created.\n");

    // 2. bind it with your address structure
    if(bind(serverFd, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr)) < 0)
    {
        perror("ERROR: Couldn't bind the socket");\
        exit(-1);
    }

    // 3. Listen for incoming connections
    if(listen(serverFd, 10) < 0)
    {
        perror("listen failed");
        exit(-1);
    }
    else
        printf("Listen successful.\n");

    FD_SET(serverFd, &fdSet);
    maxFd = serverFd;

    printf("Waiting...\n");
    memset(stCache_LRU, 0, 10 * sizeof(stCache));
    socklen = sizeof(remoteAddr);

    while(1)
    {
        fd_set tmpFdSet;
        int iterFd;
        int clientFd;
        int proxyFd;
        int status;
        int victim_idx = 0;
        char buff[BUFFER_SIZE];
        char hostname[MAX_STR_LEN];
        char url[MAX_STR_LEN];
        char file_name[MAX_STR_LEN];
        time_t curTime;
        struct addrinfo hints;
        struct addrinfo *servinfo;
        struct tm *curTimeGmt;
        FILE *readfp;

        tmpFdSet = fdSet;

        // 4. Use select() to handle multiple connections 
        if(select(maxFd+1, &tmpFdSet, NULL, NULL, NULL) == -1)
        {
            perror("ERROR: Select Failed.");
            exit(-1);
        }

        for(iterFd = 0; iterFd <= maxFd; iterFd++)
        {
            if(FD_ISSET(iterFd, &tmpFdSet))
            {
                if (iterFd == serverFd)
                {
                    // 5. Use accept() to accept incoming connections
                    clientFd = accept(serverFd, (struct sockaddr *)&remoteAddr, &socklen);
                    maxFd = (clientFd > maxFd) ? clientFd : maxFd;
                    FD_SET(clientFd, &tmpFdSet);
                    if (clientFd == -1)
                    {
                        perror("ERROR: accept failed.");
                        continue;
                    }
                }
                else
                {
                    memset(buff, 0, BUFFER_SIZE);
                    clientFd = iterFd;
                    // recv from client
                    if (recv(clientFd, buff, BUFFER_SIZE, 0) < 0)
                    {
                        perror("ERROR: recv failed.");
                        close(clientFd);
                        return -1;
                    }
                    printf ("Message sent from Client:\n%s\n", buff);

                    memset(file_name, 0, sizeof(file_name));
                    memset(url, 0, sizeof(url));
                    memset(hostname, 0, sizeof(hostname));

                    // read request
                    if (read_request(buff, file_name, url, hostname, &portAddr) == -1)
                    {
                        send_message_to_client(400, clientFd);
                        close(clientFd);
                        return 1;
                    }
                    if ((proxyFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        perror("ERROR: connecting to proxysocket failed");
                        close(clientFd);
                        return 1;
                    }
                    
                    memset(&hints, 0, sizeof(hints));
                    hints.ai_family = AF_INET;          // IPv4
                    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets 
                    // hints.ai_flags = AI_PASSIVE;      // fill in my IP for me

                    if ((status = getaddrinfo(hostname, "80", &hints, &servinfo)) != 0)
                    {
                        perror("ERROR: getaddrinfo failed.");
                        send_message_to_client(404, clientFd);
                        close(clientFd);
                        return 1;
                    }

                    if (connect(proxyFd, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
                    {
                        perror("ERROR: proxyFD connect error.");
                        send_message_to_client(404, clientFd);
                        close(proxyFd);
                        close(clientFd);
                        return 1;
                    }

                    // get current time
                    time(&curTime);
                    curTimeGmt = gmtime(&curTime);

                    int cache_idx = get_cache_idx(url);
                    // cache hit
                    if (cache_idx != -1)
                    {
                        // cache hit
                        if(check_expiration(url,curTimeGmt)==0)
                        {
                            int readlen = 0;

                            printf("File found in cache.\n");
                            printf("Sending file to the client...\n");
                            sprintf(stCache_LRU[cache_idx].modified_timestamp, "%s, %02d %s %4d %02d:%02d:%02d GMT", 
                                    strDay[curTimeGmt->tm_wday], curTimeGmt->tm_mday, strMonth[curTimeGmt->tm_mon], 
                                    curTimeGmt->tm_year+1900, curTimeGmt->tm_hour, curTimeGmt->tm_min, curTimeGmt->tm_sec);
                            readfp = fopen(stCache_LRU[cache_idx].file_name,"r");
                            memset(buff, 0, BUFFER_SIZE);
                            while ((readlen = fread(buff, 1, BUFFER_SIZE, readfp)) > 0)
                            {
                                send(clientFd, buff, readlen, 0);
                            }
                            printf("File sent to the client.\n");
                            fclose(readfp);
                        }
                        // cache hit but expired
                        else{
                            int recvlen;
                            char modified[100];
                            char modified_request[BUFFER_SIZE];
                            char* expires;

                            printf("Cache hit, but file expired.\n");
                            printf("Requesting file from the remote server.\n");
                            memset(modified, 0, 100);
                            sprintf(modified, "If-Modified-Since: %s\r\n\r\n", stCache_LRU[cache_idx].modified_timestamp);
                            memset(modified_request, 0, BUFFER_SIZE);
                            memcpy(modified_request, buff, strlen(buff)-2);

                            strcat(modified_request, modified);
                            printf ("Sending HTTP query to web server\n");
                            printf("%s\n", modified_request);
                            send(proxyFd, modified_request, strlen(modified_request), 0);
                            memset(buff, 0, BUFFER_SIZE);
                            recvlen = recv(proxyFd, buff, BUFFER_SIZE, 0);
                            expires = strstr(buff, "Expires: ");    // search for a word to modify
                            if (expires != NULL)
                            {
                                memcpy(stCache_LRU[cache_idx].expire, expires + 9, 29);
                            }
                            else
                            {
                                sprintf(stCache_LRU[cache_idx].expire, "%s, %02d %s %4d %02d:%02d:%02d GMT", 
                                        strDay[curTimeGmt->tm_wday], curTimeGmt->tm_mday, strMonth[curTimeGmt->tm_mon], 
                                        curTimeGmt->tm_year+1900, curTimeGmt->tm_hour, curTimeGmt->tm_min, curTimeGmt->tm_sec);
                            }

                            if (recvlen <= 0)
                            {
                                perror("ERROR: recv() failed.");
                            }
                            else
                            {
                                printf("Printing HTTP response \n %s\n", buff);
                                if (strncmp(buff+9, "304", 3) == 0)
                                {
                                    int readlen = 0;
                                    printf("File is still up to date. Sending file in cache.\n");

                                    readfp = fopen(stCache_LRU[cache_idx].file_name,"r");
                                    memset(buff, 0, BUFFER_SIZE);
                                    while ((readlen=fread(buff, 1, BUFFER_SIZE, readfp))>0)
                                    {
                                        send(clientFd, buff, readlen, 0);
                                    }
                                    fclose(readfp);
                                }
                                else if (strncmp(buff+9, "404", 3) == 0)
                                {
                                    send_message_to_client(404, clientFd);
                                }
                                else if (strncmp(buff+9, "200", 3) == 0)
                                {
                                    printf("New file received from server.\n");
                                    printf("Sending file to client.\n");
                                    send(clientFd, buff, recvlen, 0);
                                    remove(stCache_LRU[cache_idx].file_name);

                                    expires = strstr(buff, "Expires: ");
                                    if (expires!=NULL)
                                    {
                                        memcpy(stCache_LRU[cache_idx].expire, expires+9, 29);
                                    }
                                    else
                                    {
                                        sprintf(stCache_LRU[cache_idx].expire, "%s, %02d %s %4d %02d:%02d:%02d GMT", 
                                                strDay[curTimeGmt->tm_wday], curTimeGmt->tm_mday, strMonth[curTimeGmt->tm_mon], 
                                                curTimeGmt->tm_year+1900, curTimeGmt->tm_hour, curTimeGmt->tm_min, curTimeGmt->tm_sec);
                                    }

                                    sprintf(stCache_LRU[cache_idx].modified_timestamp, "%s, %02d %s %4d %02d:%02d:%02d GMT", 
                                            strDay[curTimeGmt->tm_wday], curTimeGmt->tm_mday, strMonth[curTimeGmt->tm_mon], 
                                            curTimeGmt->tm_year+1900, curTimeGmt->tm_hour, curTimeGmt->tm_min, curTimeGmt->tm_sec);
                                    
                                    FILE *writeFp = fopen(stCache_LRU[cache_idx].file_name, "w");
                                    fwrite(buff, 1, recvlen, writeFp);
                                    memset(buff, 0, BUFFER_SIZE);

                                    while ((recvlen = recv(proxyFd, buff, BUFFER_SIZE, 0)) > 0)
                                    {
                                        send(clientFd, buff, recvlen, 0);
                                        fwrite(buff, 1, recvlen, writeFp);
                                    }
                                    fclose(writeFp);
                                }
                            }
                        }
                    }
                    // cache missed
                    else
                    {
                        char reqBuff[BUFFER_SIZE];
                        int sendLen = 0;
                        int recvlen = 0;
                        char* ptrToken;

                        printf("Cache missed.\n");
                        printf("Passing HTTP request to the web server\n");
                        sprintf(reqBuff, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", file_name, hostname);
                        puts(reqBuff);

                        if ((sendLen = send(proxyFd, reqBuff, strlen(reqBuff), 0)) < 0)
                        {
                            perror("ERROR: send() request to the web server failed.");
                            close(proxyFd);
                            return 1;
                        }
                        printf("Request passed to the web server.\n");
                        memset(buff, 0, BUFFER_SIZE);

                        victim_idx = get_victim_idx(victim_idx);

                        memset(&stCache_LRU[victim_idx], 0, sizeof(stCache));
                        set_cache_entry_valid(victim_idx);

                        ptrToken = strtok(file_name, "/");
                        while (ptrToken != NULL)
                        {
                            strcpy(stCache_LRU[victim_idx].file_name, ptrToken);
                            ptrToken = strtok(NULL, "/");
                        }

                        memcpy(stCache_LRU[victim_idx].url, url, MAX_STR_LEN);
                        sprintf(stCache_LRU[victim_idx].modified_timestamp, "%s, %02d %s %d %02d:%02d:%02d GMT", 
                                strDay[curTimeGmt->tm_wday],curTimeGmt->tm_mday, strMonth[curTimeGmt->tm_mon], 
                                curTimeGmt->tm_year+1900, curTimeGmt->tm_hour, curTimeGmt->tm_min, curTimeGmt->tm_sec);

                        remove(stCache_LRU[victim_idx].file_name);
                        FILE *writeFp = fopen(stCache_LRU[victim_idx].file_name, "w");
                        if (writeFp == NULL)
                        {
                            printf("ERROR: Cache didn't get saved properly.\n");
                            return 1;
                        }
                        while ((recvlen = recv(proxyFd, buff, BUFFER_SIZE, 0)) > 0)
                        {
                            if (send(clientFd, buff, recvlen, 0) < 0)
                            {
                                perror("ERROR: Client send() failed.");
                                return 1;
                            }
                            fwrite(buff, 1, recvlen, writeFp);
                            memset(buff, 0, BUFFER_SIZE);
                        }
                        send(clientFd, buff, 0, 0);
                        printf("Recieved response from the web server.\n");
                        printf("Passed the file to the client.\n");
                        printf("Sent file to the client \n*******************************************************\n");
                        fclose(writeFp);

                        readfp = fopen(stCache_LRU[victim_idx].file_name, "r");
                        fread(buff, 1, BUFFER_SIZE, readfp);
                        fclose(readfp);

                        char* expires;
                        expires = strstr(buff, "Expires: ");
                        if (expires != NULL)
                        {
                            memcpy(stCache_LRU[victim_idx].expire, expires + 9, 29);
                        }
                        else
                        {
                            sprintf(stCache_LRU[victim_idx].expire, "%s, %02d %s %4d %02d:%02d:%02d GMT", 
                                    strDay[curTimeGmt->tm_wday], curTimeGmt->tm_mday, strMonth[curTimeGmt->tm_mon], 
                                    curTimeGmt->tm_year+1900, (curTimeGmt->tm_hour), (curTimeGmt->tm_min)+2, curTimeGmt->tm_sec);
                        }
                    }   
                    
                    // clear fd
                    FD_CLR(clientFd, &fdSet);
                    close(clientFd);
                    close(proxyFd);
                    display_cache_LRU();
                }
            }

        }
    }
}

