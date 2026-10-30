#include "common.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

struct Server {
  char ip[255];
  int port;
};

#define MAX_SERVERS 32

struct ClientTask {
    struct Server server;
    uint64_t begin, end, mod;
    uint64_t result;
    bool success;
};

// Читаем список серверов из файла
static int ReadServers(const char *path, struct Server *to) {
    FILE *file = fopen(path, "r");

    if (!file) {
        perror("servers.txt");
        return -1;
    }

    char line[512];
    int count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char ip[255], extra;
        int port;

        if (sscanf(line, " %254[^:]:%d %c",
                   ip, &port, &extra) != 2 ||
            port < 1 || port > 65535 ||
            count >= MAX_SERVERS) {
            fprintf(stderr, "Bad server entry: %s", line);
            fclose(file);
            return -1;
        }

        struct in_addr address;

        if (inet_pton(AF_INET, ip, &address) != 1) {
            fprintf(stderr, "Invalid IP: %s\n", ip);
            fclose(file);
            return -1;
        }

        snprintf(to[count].ip, sizeof(to[count].ip),
                 "%s", ip);
        to[count].port = port;
        count++;
    }

    fclose(file);

    if (count == 0)
        fprintf(stderr, "Servers file is empty\n");

    return count ? count : -1;
}

// Отправляем все байты сообщения
static int SendAll(int fd, const void *data, size_t size) {
    size_t sent = 0;

    while (sent < size) {
        ssize_t n = send(fd, (const char *)data + sent,
                         size - sent, 0);
        if (n <= 0)
            return -1;

        sent += (size_t)n;
    }

    return 0;
}

// Получаем все байты ответа
static int RecvAll(int fd, void *data, size_t size) {
    size_t received = 0;

    while (received < size) {
        ssize_t n = recv(fd, (char *)data + received,
                         size - received, 0);
        if (n <= 0)
            return -1;

        received += (size_t)n;
    }

    return 0;
}

// Работа отдельного потока клиента
static void *ClientWorker(void *arg) {
    struct ClientTask *job = arg;
    job->success = false;

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        perror("socket");
        return NULL;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)job->server.port);

    if (inet_pton(AF_INET, job->server.ip,
                  &addr.sin_addr) != 1 ||
        connect(fd, (struct sockaddr *)&addr,
                sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return NULL;
    }

    uint64_t request[3] = {
        job->begin, job->end, job->mod
    };

    if (SendAll(fd, request, sizeof(request)) == 0 &&
        RecvAll(fd, &job->result,
                sizeof(job->result)) == 0) {
        job->success = true;
    } else {
        fprintf(stderr, "Network error: %s:%d\n",
                job->server.ip, job->server.port);
    }

    close(fd);
    return NULL;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

int main(int argc, char **argv) {
 uint64_t k = UINT64_MAX;
 uint64_t mod = UINT64_MAX;
  char servers[255] = {'\0'}; // TODO: explain why 255

  while (true) {

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here
        break;
      case 2:
        // TODO: your code here
        snprintf(servers, sizeof(servers), "%s", optarg);
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == UINT64_MAX || mod == UINT64_MAX ||
    !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

if (mod == 0) {
    fprintf(stderr, "mod must be positive\n");
    return 1;
}

struct Server to[MAX_SERVERS];
int servers_num = ReadServers(servers, to);

if (servers_num < 0)
    return 1;

if (k == 0) {
    printf("answer: %" PRIu64 "\n",
           (uint64_t)(1 % mod));
    return 0;
}

signal(SIGPIPE, SIG_IGN);

int active = k < (uint64_t)servers_num
             ? (int)k : servers_num;

uint64_t base = k / (uint64_t)active;
uint64_t extra = k % (uint64_t)active;

pthread_t threads[MAX_SERVERS];
struct ClientTask jobs[MAX_SERVERS];

uint64_t next = 1;
int created = 0;

// Распределяем вычисления между серверами
for (int i = 0; i < active; i++) {
    uint64_t length =
        base + ((uint64_t)i < extra ? 1 : 0);

    jobs[i].server = to[i];
    jobs[i].begin = next;
    jobs[i].end = next + length - 1;
    jobs[i].mod = mod;
    jobs[i].success = false;

    next += length;

    if (pthread_create(&threads[i], NULL,
                       ClientWorker, &jobs[i])) {
        fprintf(stderr, "pthread_create failed\n");
        break;
    }

    created++;
}

// Получаем ответы и объединяем результаты
uint64_t answer = 1 % mod;
bool all_ok = (created == active);

for (int i = 0; i < created; i++) {
    pthread_join(threads[i], NULL);

    if (!jobs[i].success) {
        all_ok = false;
        continue;
    }

    printf("Server %s:%d [%" PRIu64
           "..%" PRIu64 "] = %" PRIu64 "\n",
           jobs[i].server.ip,
           jobs[i].server.port,
           jobs[i].begin,
           jobs[i].end,
           jobs[i].result);

    answer = MultModulo(answer,
                        jobs[i].result, mod);
}

if (!all_ok) {
    fprintf(stderr, "One or more servers failed\n");
    return 1;
}

printf("answer: %" PRIu64 "\n", answer);

return 0;
}

