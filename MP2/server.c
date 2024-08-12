// This is the server code for MP2
// author: rohan.dalvi@tamu.edu
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CustomStruct.h"

int ClientCount = 0;
struct ClientInfo *activeClients;

int CheckUsernameAvailability(char name[]) {
    int count = 0;
    int result = 0;
    for (count = 0; count < ClientCount; count++) {
        if (!strcmp(name, activeClients[count].username)) {
            printf("Another client with the username '%s' is trying to connect, but the username already exists. Rejecting it.\n", name);
            result = 1;
            break;
        }
    }
    return result;
}

void SendAcknowledgement(int connectionFd) {
    struct Message AckMessage;
    struct Header AckHeader;
    struct MessageAttribute AckAttribute;
    int count = 0;
    char temp[180];

    AckHeader.version = 3;
    AckHeader.type = 7;

    AckAttribute.type = 4;

    temp[0] = (char)(((int)'0') + ClientCount);
    temp[1] = ' ';
    temp[2] = '\0';
    for (count = 0; count < ClientCount - 1; count++) {
        strcat(temp, activeClients[count].username);
        if (count != ClientCount - 2)
            strcat(temp, ",");
    }
    AckAttribute.length = strlen(temp) + 1;
    strcpy(AckAttribute.payload, temp);
    AckMessage.header = AckHeader;
    AckMessage.attribute[0] = AckAttribute;

    write(connectionFd, (void *)&AckMessage, sizeof(AckMessage));
}

void SendRejection(int connectionFd, int code) {
    struct Message RejectionMessage;
    struct Header RejectionHeader;
    struct MessageAttribute RejectionAttribute;
    char temp[32];

    RejectionHeader.version = 3;
    RejectionHeader.type = 5;

    RejectionAttribute.type = 1;

    if (code == 1)
        strcpy(temp, "Username is incorrect");

    if (code == 2)
        strcpy(temp, "Client count exceeded");

    RejectionAttribute.length = strlen(temp);
    strcpy(RejectionAttribute.payload, temp);

    RejectionMessage.header = RejectionHeader;
    RejectionMessage.attribute[0] = RejectionAttribute;

    write(connectionFd, (void *)&RejectionMessage, sizeof(RejectionMessage));

    close(connectionFd);
}

void NotifyOnline(fd_set master, int serverSocket, int connectionFd, int maxFd) {
    struct Message OnlineMessage;
    int j;
    printf("Server accepted the client: %s\n", activeClients[ClientCount - 1].username);
    OnlineMessage.header.version = 3;
    OnlineMessage.header.type = 8;
    OnlineMessage.attribute[0].type = 2;
    strcpy(OnlineMessage.attribute[0].payload, activeClients[ClientCount - 1].username);

    for (j = 0; j <= maxFd; j++) {
        if (FD_ISSET(j, &master)) {
            if (j != serverSocket && j != connectionFd) {
                if ((write(j, (void *)&OnlineMessage, sizeof(OnlineMessage))) == -1) {
                    perror("send");
                }
            }
        }
    }
}

void NotifyOffline(fd_set master, int clientSocket, int serverSocket, int connectionFd, int maxFd, int ClientCount) {
    struct Message OfflineMessage;
    int index, j;
    for (index = 0; index < ClientCount; index++) {
        if (activeClients[index].fd == clientSocket) {
            OfflineMessage.attribute[0].type = 2;
            strcpy(OfflineMessage.attribute[0].payload, activeClients[index].username);
        }
    }

    printf("Socket %d belonging to user '%s' is disconnected\n", clientSocket, OfflineMessage.attribute[0].payload);
    OfflineMessage.header.version = 3;
    OfflineMessage.header.type = 6;

    for (j = 0; j <= maxFd; j++) {
        if (FD_ISSET(j, &master)) {
            if (j != clientSocket && j != serverSocket) {
                if ((write(j, (void *)&OfflineMessage, sizeof(OfflineMessage))) == -1) {
                    perror("ERROR: Sending");
                }
            }
        }
    }
}

int ValidateClient(int connectionFd, int maxAllowedClients) {
    struct Message JoinMessage;
    struct MessageAttribute JoinAttribute;
    char temp[30];

    int status = 0;
    read(connectionFd, (struct Message *)&JoinMessage, sizeof(JoinMessage));

    JoinAttribute = JoinMessage.attribute[0];

    strcpy(temp, JoinAttribute.payload);

    if (ClientCount == maxAllowedClients) {
        status = 2;
        printf("A new client is trying to connect, but the client count exceeded. Rejecting it\n");
        SendRejection(connectionFd, 2); // 2- indicates client count exceeded.
        return status;
    }

    status = CheckUsernameAvailability(temp);
    if (status == 1)
        SendRejection(connectionFd, 1); // 1- indicates client already present
    else {
        strcpy(activeClients[ClientCount].username, temp);
        activeClients[ClientCount].fd = connectionFd;
        activeClients[ClientCount].ClientCount = ClientCount;
        ClientCount = ClientCount + 1;
        SendAcknowledgement(connectionFd);
    }
    return status;
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Please use the correct format: %s <hostname> <port> <max_clients>\n", argv[0]);
        exit(0);
    }

    struct Message BroadcastMessage, OfflineMessage;
    struct Message ClientMessage;
    struct MessageAttribute ClientAttribute;

    int serverSocket, connectionFd, m, k;
    unsigned int length;
    int clientStatus = 0;
    struct sockaddr_in serverAddress, *clientInfo;
    struct hostent *hostEntry;

    fd_set master;
    fd_set readSet;
    int maxFd, temp, i = 0, j = 0, x = 0, y, bytesRead, maxAllowedClients = 0;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("ERROR: Couldn't create a socket");
        exit(0);
    } else
        printf("Server socket is created.\n");

    bzero(&serverAddress, sizeof(serverAddress));

    int enable = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("ERROR: Setsockopt\n");
        exit(1);
    }

    serverAddress.sin_family = AF_INET;
    hostEntry = gethostbyname(argv[1]);
    memcpy(&serverAddress.sin_addr.s_addr, hostEntry->h_addr, hostEntry->h_length);
    serverAddress.sin_port = htons(atoi(argv[2]));

    maxAllowedClients = atoi(argv[3]);

    activeClients = (struct ClientInfo *)malloc(maxAllowedClients * sizeof(struct ClientInfo));
    clientInfo = (struct sockaddr_in *)malloc(maxAllowedClients * sizeof(struct sockaddr_in));

    if ((bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress))) != 0) {
        perror("ERROR: Couldn't bind the socket\n");
        exit(0);
    } else
        printf("Binding successful.\n");

    if ((listen(serverSocket, maxAllowedClients)) != 0) {
        perror("ERROR: Couldn't listen.\n");
        exit(0);
    } else
        printf("Listening successful.\n");

    FD_SET(serverSocket, &master);
    maxFd = serverSocket;

    for (;;) {
        readSet = master;
        if (select(maxFd + 1, &readSet, NULL, NULL, NULL) == -1) {
            perror("ERROR: select");
            exit(4);
        }

        for (i = 0; i <= maxFd; i++) {
            if (FD_ISSET(i, &readSet)) {
                if (i == serverSocket) {
                    // Incoming connection
                    length = sizeof(clientInfo[ClientCount]);
                    connectionFd = accept(serverSocket, (struct sockaddr *)&clientInfo[ClientCount], &length);

                    // Checking the validity of accept
                    if (connectionFd < 0) {
                        perror("ERROR: Couldn't accept \n");
                        exit(0);
                    } else {
                        temp = maxFd;
                        FD_SET(connectionFd, &master);
                        if (connectionFd > maxFd) {
                            maxFd = connectionFd;
                        }
                        clientStatus = ValidateClient(connectionFd, maxAllowedClients);
                        if (clientStatus == 0)
                            NotifyOnline(master, serverSocket, connectionFd, maxFd);

                        else if (clientStatus == 1) {
                            // Username already present. Restore maxFD and remove it from the set
                            maxFd = temp;
                            FD_CLR(connectionFd, &master);
                        } else {
                            // Username already present. Restore maxFD and remove it from the set
                            maxFd = temp;
                            FD_CLR(connectionFd, &master); // clear connectionFd to remove this client
                        }
                    }
                } else {
                    // OLD connections
                    if ((bytesRead = read(i, (struct Message *)&ClientMessage, sizeof(ClientMessage))) <= 0) {
                        // got error or connection closed by the client
                        if (bytesRead == 0)
                            NotifyOffline(master, i, serverSocket, connectionFd, maxFd, ClientCount);
                        else
                            perror("ERROR In receiving");

                        // Cleaning up after closing the erroneous socket
                        close(i);
                        FD_CLR(i, &master); // remove from the master set
                        for (k = 0; k < ClientCount; k++) {
                            if (activeClients[k].fd == i) {
                                m = k;
                                break;
                            }
                        }

                        for (x = m; x < (ClientCount - 1); x++) {
                            activeClients[x] = activeClients[x + 1];
                        }
                        ClientCount--;
                    } else {
                        int payloadLength = 0;
                        char temp[16];
                        // Checking if the existing user becomes idle
                        if (ClientMessage.header.type == 9) {
                            BroadcastMessage = ClientMessage;
                            BroadcastMessage.attribute[0].type = 2;
                            for (y = 0; y < ClientCount; y++) {
                                if (activeClients[y].fd == i) {
                                    strcpy(BroadcastMessage.attribute[0].payload, activeClients[y].username);
                                    strcpy(temp, activeClients[y].username);
                                    payloadLength = strlen(activeClients[y].username);
                                    temp[payloadLength] = '\0';
                                    printf("User '%s' is idle\n", temp);
                                }
                            }
                        } else {
                            // Non-zero-count message received from the client
                            ClientAttribute = ClientMessage.attribute[0]; // message
                            BroadcastMessage = ClientMessage;

                            BroadcastMessage.header.type = 3;
                            BroadcastMessage.attribute[1].type = 2; // username
                            payloadLength = strlen(ClientAttribute.payload);
                            strcpy(temp, ClientAttribute.payload);
                            temp[payloadLength] = '\0';

                            // Message forwarded to clients
                            for (y = 0; y < ClientCount; y++) {
                                if (activeClients[y].fd == i)
                                    strcpy(BroadcastMessage.attribute[1].payload, activeClients[y].username);
                            }
                            printf("User '%s': %s", BroadcastMessage.attribute[1].payload, temp);
                        }

                        for (j = 0; j <= maxFd; j++) {
                            if (FD_ISSET(j, &master)) {
                                // Message broadcasted to everyone except to myself and the listener
                                if (j != i && j != serverSocket) {
                                    if ((write(j, (void *)&BroadcastMessage, bytesRead)) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    close(serverSocket);

    return 0;
}

