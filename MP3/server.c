#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MAX_FILENAME_SIZE 512

int display_system_error(const char *msg) {
    perror(msg);
    exit(1);
}

int check_timeout(int fd, int time_sec) {
    fd_set rset;
    struct timeval tv;
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    tv.tv_sec = time_sec;
    tv.tv_usec = 0;
    return select(fd + 1, &rset, NULL, NULL, &tv);
}

int main(int argc, char *argv[]) {
    char data[512] = {0};
    char buffer[1024] = {0};
    int sockfd, newsock_fd;
    struct sockaddr_in client;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    char *port_str;
    port_str = malloc (sizeof port_str);
    int i, ret;
    int send_result;
    int last_block;
    unsigned short int opcode1, opcode2, block_number;
    int byte_count;
    FILE *file_pointer;
    struct addrinfo server_address_info, *address_info, *client_info, *ptr;
    int yes;
    int pid;
    int read_result;
    int blocknum = 0;
    int timeout_count = 0;
    int expected_ack = 1;
    char data_block[516] = {0};
    char data_block_cpy[516] = {0};
    char acknowledgment_packet[32] = {0};
    char filename[MAX_FILENAME_SIZE];
    char mode[512];
    int b, j;
    char ips[INET6_ADDRSTRLEN];

    yes = 1;

    memset(&server_address_info, 0, sizeof server_address_info);
    server_address_info.ai_family = AF_INET;
    server_address_info.ai_socktype = SOCK_DGRAM;
    server_address_info.ai_flags = AI_PASSIVE;

    if ((ret = getaddrinfo(NULL, argv[2], &server_address_info, &address_info)) != 0) {
        fprintf(stderr, "SERVER: %s\n", gai_strerror(ret));
        exit(1);
    }

    for (ptr = address_info; ptr != NULL; ptr = ptr->ai_next) {
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockfd < 0) {
            continue;
        }
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        break;
    }

    freeaddrinfo(address_info);

    printf("SERVER: Ready for association with clients...\n");

    while (1) {
        byte_count = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *) &client, &clientlen);

        if (byte_count < 0) {
            display_system_error("ERR: Couldn't receive data");
            return 7;
        }

        memcpy(&opcode1, &buffer, 2);
        opcode1 = ntohs(opcode1);

        pid = fork();

        if (pid == 0) { // Child process

            if (opcode1 == 1) { // RRQ processing
                bzero(filename, MAX_FILENAME_SIZE);

                for (b = 0; buffer[2 + b] != '\0'; b++) {
                    filename[b] = buffer[2 + b];
                }

                filename[b] = '\0';

                bzero(mode, 512);
                j = 0;

                for (i = b + 3; buffer[i] != '\0'; i++) {
                    mode[j] = buffer[i];
                    j++;
                }

                mode[j] = '\0';

                printf("RRQ received, filename: %s mode: %s\n", filename, mode);

                file_pointer = fopen(filename, "r");

                if (file_pointer != NULL) { // Valid Filename
                    close(sockfd);
                    *port_str = htons(0);
                    if ((ret = getaddrinfo(NULL, port_str, &server_address_info, &client_info)) != 0) {
                        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
                        return 10;
                    }

                    for (ptr = client_info; ptr != NULL; ptr = ptr->ai_next) {
                        if ((newsock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
                            display_system_error("ERR: SERVER (child): socket");
                            continue;
                        }

                        setsockopt(newsock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                        if (bind(newsock_fd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
                            close(newsock_fd);
                            display_system_error("ERR: SERVER (newsock_fd): bind");
                            continue;
                        }

                        break;
                    }

                    freeaddrinfo(client_info);

                    bzero(data_block, sizeof(data_block));
                    bzero(data, sizeof(data));
                    read_result = fread(&data, 1, 512, file_pointer);

                    if (read_result >= 0) {
                        data[read_result] = '\0';
                        printf("1: Result is %d\n", read_result);
                    }

                    if (read_result < 512) {
                        last_block = 1;
                    }

                    block_number = htons(1);
                    opcode2 = htons(3);
                    memcpy(&data_block[0], &opcode2, 2);
                    memcpy(&data_block[2], &block_number, 2);

                    for (b = 0; data[b] != '\0'; b++) {
                        data_block[b + 4] = data[b];
                    }

                    int p = 0;

                    bzero(data_block_cpy, sizeof(data_block_cpy));
                    memcpy(&data_block_cpy[0], &data_block[0], 516);

                    send_result = sendto(newsock_fd, data_block, (read_result + 4), 0, (struct sockaddr *) &client, clientlen);

                    expected_ack = 1;

                    if (send_result < 0) {
                        display_system_error("couldn't send first packet: ");
                    } else {
                        printf("Sent first block successfully.\n");
                    }

                    while (1) {
                        if (check_timeout(newsock_fd, 1) != 0) {
                            bzero(buffer, sizeof(buffer));
                            bzero(data_block, sizeof(data_block));
                            byte_count = recvfrom(newsock_fd, buffer, sizeof(buffer), 0,
                                (struct sockaddr *) &client, &clientlen);
                            timeout_count = 0;

                            if (byte_count < 0) {
                                display_system_error("Couldn't receive data\n");
                                return 6;
                            }

                            memcpy(&opcode1, &buffer[0], 2);

                            if (ntohs(opcode1) == 4) { // Opcode = 4: ACK
                                bzero(&blocknum, sizeof(blocknum));
                                memcpy(&blocknum, &buffer[2], 2);
                                blocknum = ntohs(blocknum);
                                printf("ACK %i received\n", blocknum);

                                if (blocknum == expected_ack) { // Expected ACK has arrived
                                    printf("Expected ACK has arrived\n");
                                    expected_ack = (expected_ack + 1) % 65536;

                                    if (last_block == 1) {
                                        close(newsock_fd);
                                        fclose(file_pointer);
                                        printf("SERVER: Full file is sent and connection is closed.\n");
                                        exit(5);
                                        last_block = 0;
                                    } else {
                                        bzero(data, sizeof(data));
                                        read_result = fread(&data, 1, 512, file_pointer);

                                        if (read_result >= 0) {
                                            if (read_result < 512) {
                                                last_block = 1;
                                            }

                                            data[read_result] = '\0';
                                            printf("2: Result is %d\n", read_result);

                                            block_number = htons(((blocknum + 1) % 65536));
                                            opcode2 = htons(3);
                                            memcpy(&data_block[0], &opcode2, 2);
                                            memcpy(&data_block[2], &block_number, 2);

                                            for (b = 0; data[b] != '\0'; b++) {
                                                data_block[b + 4] = data[b];
                                            }

                                            int p = 0;

                                            bzero(data_block_cpy, sizeof(data_block_cpy));
                                            memcpy(&data_block_cpy[0], &data_block[0], 516);

                                            send_result = sendto(newsock_fd, data_block, (read_result + 4), 0,
                                                (struct sockaddr *) &client, clientlen);

                                            if (send_result < 0) {
                                                display_system_error("ERR: Sendto ");
                                            }
                                        }
                                    }
                                } else {
                                    printf("Expected ACK hasn't arrived: NEACK: %d, blocknum: %d\n",
                                        expected_ack, blocknum);
                                }
                            }
                        } else {
                            timeout_count++;
                            printf("Timeout occurred.\n");

                            if (timeout_count == 10) {
                                printf("Timeout occurred 10 times. Closing socket connection with client.\n");
                                close(newsock_fd);
                                fclose(file_pointer);
                                exit(6);
                            } else {
                                bzero(data_block, sizeof(data_block));
                                memcpy(&data_block[0], &data_block_cpy[0], 516);
                                memcpy(&block_number, &data_block[2], 2);
                                block_number = htons(block_number);
                                printf("Retransmitting Data with BlockNo: %d\n", block_number);

                                send_result = sendto(newsock_fd, data_block_cpy, (read_result + 4), 0,
                                    (struct sockaddr *) &client, clientlen);

                                bzero(data_block_cpy, sizeof(data_block_cpy));
                                memcpy(&data_block_cpy[0], &data_block[0], 516);

                                if (send_result < 0) {
                                    display_system_error("ERR: Sendto ");
                                }
                            }
                        }
                    }
                } else { // Generate ERR message and send to client if file is not found
                    unsigned short int ERRCode = htons(1);
                    unsigned short int ERRoc = htons(5); // Opcode = 5: Error
                    char ERRMsg[512] = "File not found";
                    char ERRBuff[516] = {0};
                    memcpy(&ERRBuff[0], &ERRoc, 2);
                    memcpy(&ERRBuff[2], &ERRCode, 2);
                    memcpy(&ERRBuff[4], &ERRMsg, 512);
                    sendto(sockfd, ERRBuff, 516, 0, (struct sockaddr *) &client, clientlen);
                    printf("Server clean up as filename doesn't match.\n");
                    close(sockfd);
                    fclose(file_pointer);
                    exit(4);
                }
            } else if (opcode1 == 2) { // WRQ processing
                port_str = "0";

                if ((ret = getaddrinfo(NULL, port_str, &server_address_info, &client_info)) != 0) {
                    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
                    return 10;
                }

                for (ptr = client_info; ptr != NULL; ptr = ptr->ai_next) {
                    if ((newsock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
                        display_system_error("ERR: SERVER (child): socket");
                        continue;
                    }

                    setsockopt(newsock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                    if (bind(newsock_fd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
                        close(newsock_fd);
                        display_system_error("ERR: SERVER (newsock_fd): bind");
                        continue;
                    }

                    break;
                }

                freeaddrinfo(client_info);

                printf("SERVER: WRQ received from client.\n");

                FILE *fp_wr = fopen("WRQ_data.txt", "w+");

                if (fp_wr == NULL) {
                    printf("SERVER: WRQ: Problem in opening file");
                }

                block_number = htons(0);
                opcode2 = htons(4);
                bzero(acknowledgment_packet, sizeof(acknowledgment_packet));
                memcpy(&acknowledgment_packet[0], &opcode2, 2);
                memcpy(&acknowledgment_packet[2], &block_number, 2);

                send_result = sendto(newsock_fd, acknowledgment_packet, 4, 0, (struct sockaddr *) &client, clientlen); // Send ACK to client stating that server is ready to receive data

                expected_ack = 1;

                if (send_result < 0) {
                    display_system_error("WRQ ACK ERR: Sendto ");
                }

                while (1) {
                    bzero(buffer, sizeof(buffer));
                    byte_count = recvfrom(newsock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &client, &clientlen);

                    if (byte_count < 0) {
                        display_system_error("WRQ: Couldn't receive data\n");
                        return 9;
                    }

                    bzero(data, sizeof(data));
                    memcpy(&block_number, &buffer[2], 2);

                    j = 0;

                    for (i = 0; buffer[i + 4] != '\0'; i++) {
                        if (buffer[i + 4] == '\n') {
                            printf("LF character spotted.\n");
                            j++;

                            if (i - j < 0) {
                                printf("ERR: i-j is less than 0");
                            }

                            data[i - j] = '\n';
                        } else {
                            data[i - j] = buffer[i + 4];
                        }
                    }

                    fwrite(data, 1, (byte_count - 4 - j), fp_wr);
                    block_number = ntohs(block_number);
                    printf("SERVER: Received data block #%d\n", expected_ack);

                    if (expected_ack == block_number) {
                        printf("SERVER: Received data block #%d\n", expected_ack);
                        printf("SERVER: Expected data block received.\n");

                        opcode2 = htons(4);
                        block_number = ntohs(expected_ack);
                        bzero(acknowledgment_packet, sizeof(acknowledgment_packet));
                        memcpy(&acknowledgment_packet[0], &opcode2, 2);
                        memcpy(&acknowledgment_packet[2], &block_number, 2);

                        printf("SERVER: Sent ACK #%d\n", htons(expected_ack));
                        send_result = sendto(newsock_fd, acknowledgment_packet, 4, 0, (struct sockaddr *) &client, clientlen);

                        if (byte_count < 516) {
                            printf("Last data block has arrived. Closing client connection and cleaning resources. \n");
                            close(newsock_fd);
                            fclose(fp_wr);
                            exit(9);
                        }

                        expected_ack = (expected_ack + 1) % 65536;
                    }
                }
            }
        }
    }
}
