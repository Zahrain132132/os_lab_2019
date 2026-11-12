
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <buffer_size>\n", argv[0]);
        return 1;
    }

    char *end;
    long port = strtol(argv[1], &end, 10);

    if (*argv[1] == '\0' || *end != '\0' ||
        port < 1 || port > 65535) {
        fprintf(stderr, "Invalid port\n");
        return 1;
    }

    long size = strtol(argv[2], &end, 10);

    if (*argv[2] == '\0' || *end != '\0' ||
        size < 1 || size > 65507) {
        fprintf(stderr, "Invalid buffer size\n");
        return 1;
    }

    size_t buffer_size = (size_t)size;

    char *mesg = malloc(buffer_size + 1);
    if (mesg == NULL) {
        perror("malloc");
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char ipadr[INET_ADDRSTRLEN];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        free(mesg);
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((unsigned short)port);

    if (bind(sockfd, (struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind");
        close(sockfd);
        free(mesg);
        return 1;
    }

    printf("UDP server listening on port %ld\n", port);
    fflush(stdout);

    while (1) {
        socklen_t len = sizeof(cliaddr);

        ssize_t n = recvfrom(
            sockfd,
            mesg,
            buffer_size,
            0,
            (struct sockaddr *)&cliaddr,
            &len
        );

        if (n < 0) {
            perror("recvfrom");
            break;
        }

        mesg[n] = '\0';

        inet_ntop(AF_INET, &cliaddr.sin_addr,
                  ipadr, sizeof(ipadr));

        printf("REQUEST %s FROM %s : %d\n",
               mesg, ipadr, ntohs(cliaddr.sin_port));
        fflush(stdout);

        if (sendto(sockfd, mesg, (size_t)n, 0,
                   (struct sockaddr *)&cliaddr,
                   len) < 0) {
            perror("sendto");
            break;
        }
    }

    close(sockfd);
    free(mesg);
    return 0;
}
