// udpserver.c
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <bufsize> <max_connections>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int bufsize = atoi(argv[2]);
    // max_connections не используется для UDP, но оставим для совместимости

    int sockfd, n;
    char *mesg = malloc(bufsize);
    char ipadr[16];
    if (!mesg) {
        perror("malloc");
        exit(1);
    }

    struct sockaddr_in servaddr;
    struct sockaddr_in cliaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        free(mesg);
        exit(1);
    }

    memset(&servaddr, 0, SLEN);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
        perror("bind problem");
        free(mesg);
        exit(1);
    }
    printf("SERVER starts on port %d...\n", port);

    while (1) {
        unsigned int len = SLEN;

        if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
            perror("recvfrom");
            free(mesg);
            close(sockfd);
            exit(1);
        }
        mesg[n] = 0;

        printf("REQUEST '%s' FROM %s : %d\n",
               mesg,
               inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
               ntohs(cliaddr.sin_port));

        if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
            perror("sendto");
            free(mesg);
            close(sockfd);
            exit(1);
        }
    }

    free(mesg);
    close(sockfd);
    return 0;
}