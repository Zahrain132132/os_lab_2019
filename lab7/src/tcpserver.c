
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
        size < 1 || size > 1048576) {
        fprintf(stderr, "Invalid buffer size\n");
        return 1;
    }

    size_t buffer_size = (size_t)size;
    char *buf = malloc(buffer_size);

    if (buf == NULL) {
        perror("malloc");
        return 1;
    }

    int lfd, cfd;
    ssize_t nread;
    struct sockaddr_in servaddr, cliaddr;

    if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        free(buf);
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((unsigned short)port);

    if (bind(lfd, (struct sockaddr *)&servaddr,
             sizeof(servaddr)) < 0) {
        perror("bind");
        close(lfd);
        free(buf);
        return 1;
    }

    if (listen(lfd, 5) < 0) {
        perror("listen");
        close(lfd);
        free(buf);
        return 1;
    }

    printf("TCP server listening on port %ld\n", port);
    fflush(stdout);

    while (1) {
        socklen_t clilen = sizeof(cliaddr);

        cfd = accept(lfd, (struct sockaddr *)&cliaddr,
                     &clilen);
        if (cfd < 0) {
            perror("accept");
            break;
        }

        printf("connection established\n");
        fflush(stdout);

        while ((nread = read(cfd, buf, buffer_size)) > 0) {
            if (write(STDOUT_FILENO, buf, (size_t)nread) < 0) {
                perror("write");
                break;
            }
        }

        if (nread < 0) {
            perror("read");
        }

        close(cfd);
    }

    close(lfd);
    free(buf);
    return 0;
}

