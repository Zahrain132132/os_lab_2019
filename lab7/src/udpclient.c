
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <IP> <port> <buffer_size>\n", argv[0]);
        return 1;
    }

    char *end;
    long port = strtol(argv[2], &end, 10);

    if (*argv[2] == '\0' || *end != '\0' ||
        port < 1 || port > 65535) {
        fprintf(stderr, "Invalid port\n");
        return 1;
    }

    long size = strtol(argv[3], &end, 10);

    if (*argv[3] == '\0' || *end != '\0' ||
        size < 1 || size > 65507) {
        fprintf(stderr, "Invalid buffer size\n");
        return 1;
    }

    size_t buffer_size = (size_t)size;

    char *sendline = malloc(buffer_size);
    char *recvline = malloc(buffer_size + 1);

    if (sendline == NULL || recvline == NULL) {
        perror("malloc");
        free(sendline);
        free(recvline);
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("socket");
        free(sendline);
        free(recvline);
        return 1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((unsigned short)port);

    if (inet_pton(AF_INET, argv[1],
                  &servaddr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address\n");
        close(sockfd);
        free(sendline);
        free(recvline);
        return 1;
    }

    printf("Enter string\n");
    fflush(stdout);

    ssize_t nread;

    while ((nread = read(STDIN_FILENO, sendline,
                         buffer_size)) > 0) {

        ssize_t sent = sendto(
            sockfd, sendline, (size_t)nread, 0,
            (struct sockaddr *)&servaddr,
            sizeof(servaddr)
        );

        if (sent != nread) {
            perror("sendto");
            break;
        }

        ssize_t received = recvfrom(
            sockfd, recvline, buffer_size,
            0, NULL, NULL
        );

        if (received < 0) {
            perror("recvfrom");
            break;
        }

        recvline[received] = '\0';

        printf("REPLY FROM SERVER= %s", recvline);

        if (received == 0 ||
            recvline[received - 1] != '\n') {
            putchar('\n');
        }

        fflush(stdout);
    }

    if (nread < 0) {
        perror("read");
    }

    close(sockfd);
    free(sendline);
    free(recvline);

    return 0;
}
