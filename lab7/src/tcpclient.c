#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <bufsize> <message>\n", argv[0]);
        exit(1);
    }

    int fd;
    int nread;
    int bufsize = atoi(argv[3]);
    char *buf = malloc(bufsize);
    if (!buf) {
        perror("malloc");
        exit(1);
    }

    struct sockaddr_in servaddr;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creating");
        free(buf);
        exit(1);
    }

    memset(&servaddr, 0, SIZE);
    servaddr.sin_family = AF_INET;

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("bad address");
        free(buf);
        exit(1);
    }

    servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
        perror("connect");
        free(buf);
        exit(1);
    }

    if (argv[4] && strlen(argv[4]) > 0) {
        write(fd, argv[4], strlen(argv[4]));
        close(fd);
        free(buf);
        exit(0);
    }

    write(1, "Input message to send (Ctrl+D to stop):\n", 38);
    while ((nread = read(0, buf, bufsize)) > 0) {
        if (write(fd, buf, nread) < 0) {
            perror("write");
            free(buf);
            exit(1);
        }
    }

    close(fd);
    free(buf);
    exit(0);
}