
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef unsigned long long ull;

// Общий результат всех потоков
ull result = 1;

// Мьютекс для защиты общего результата
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Данные, передаваемые каждому потоку
typedef struct {
    ull start;
    ull end;
    ull mod;
    ull partial;
} ThreadArgs;

// Умножение по модулю без переполнения 64 бит
ull mul_mod(ull a, ull b, ull mod) {
    return (ull)(((__uint128_t)a * b) % mod);
}

// Функция, которую выполняет каждый поток
void *factorial_part(void *arg) {
    ThreadArgs *data = (ThreadArgs *)arg;

    ull local = 1 % data->mod;

    for (ull i = data->start; i <= data->end; i++) {
        local = mul_mod(local, i, data->mod);
    }

    data->partial = local;

    // Защищаем общий результат мьютексом
    pthread_mutex_lock(&mutex);

    result = mul_mod(result, local, data->mod);

    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char *argv[]) {

    // Проверяем аргументы командной строки
    if (argc != 5 ||
        strcmp(argv[1], "-k") != 0 ||
        strncmp(argv[3], "--pnum=", 7) != 0 ||
        strncmp(argv[4], "--mod=", 6) != 0) {

        printf("Usage: %s -k 10 --pnum=4 --mod=10\n",
               argv[0]);
        return 1;
    }

    if (argv[2][0] == '-' ||
        argv[3][7] == '-' ||
        argv[4][6] == '-') {
        printf("Negative values are not allowed\n");
        return 1;
    }

    ull k = strtoull(argv[2], NULL, 10);
    int pnum = atoi(argv[3] + 7);
    ull mod = strtoull(argv[4] + 6, NULL, 10);

    if (pnum <= 0 || mod == 0) {
        printf("Invalid parameters\n");
        return 1;
    }

    pthread_t *threads = calloc(pnum, sizeof(pthread_t));
    ThreadArgs *args = calloc(pnum, sizeof(ThreadArgs));

    if (threads == NULL || args == NULL) {
        printf("Memory allocation failed\n");
        free(threads);
        free(args);
        return 1;
    }

    result = 1 % mod;

    // Делим числа от 1 до k между потоками
    ull base = k / pnum;
    ull remainder = k % pnum;
    ull next = 1;

    for (int i = 0; i < pnum; i++) {
        ull count = base + ((ull)i < remainder);

        args[i].start = next;
        args[i].end = next + count - 1;
        args[i].mod = mod;

        next += count;

        // Запускаем поток
        if (pthread_create(&threads[i], NULL,
                           factorial_part, &args[i]) != 0) {
            printf("Thread creation failed\n");
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(args);
            return 1;
        }
    }

    // Дожидаемся завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);

        printf("Thread %d: [%llu..%llu], partial = %llu\n",
               i + 1,
               args[i].start,
               args[i].end,
               args[i].partial);
    }

    printf("\n%llu! mod %llu = %llu\n",
           k, mod, result);

    pthread_mutex_destroy(&mutex);
    free(threads);
    free(args);

    return 0;
}
