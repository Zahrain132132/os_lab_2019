#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <getopt.h>
#include <pthread.h>

#include "sum.h"
#include "utils.h"

/*
 * Проверка числовых аргументов.
 */
static bool parse_number(
    const char *text,
    unsigned int minimum,
    unsigned int *result
) {
    if (text[0] == '\0' || text[0] == '-') {
        return false;
    }

    char *end = NULL;
    errno = 0;

    unsigned long value = strtoul(text, &end, 10);

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        value < minimum ||
        value > INT_MAX) {
        return false;
    }

    *result = (unsigned int)value;

    return true;
}

/*
 * Функция, которую выполняет каждый поток.
 */
static void *ThreadSum(void *context) {

    struct SumArgs *args = context;

    /*
     * Вычисляем частичную сумму.
     */
    args->result = Sum(args);

    return NULL;
}

/*
 * Разница между двумя моментами времени
 * в миллисекундах.
 */
static double elapsed_ms(
    const struct timespec *start,
    const struct timespec *finish
) {
    double seconds =
        (double)(finish->tv_sec - start->tv_sec);

    double nanoseconds =
        (double)(finish->tv_nsec - start->tv_nsec);

    return seconds * 1000.0 + nanoseconds / 1000000.0;
}

int main(int argc, char **argv) {

    unsigned int threads_num = 0;
    unsigned int array_size = 0;
    unsigned int seed = 0;

    bool has_threads = false;
    bool has_size = false;
    bool has_seed = false;

    /*
     * Обработка аргументов командной строки.
     */
    static struct option options[] = {
        {"threads_num", required_argument, 0, 't'},
        {"seed", required_argument, 0, 's'},
        {"array_size", required_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    int option;

    while ((option = getopt_long(
        argc, argv, "", options, NULL
    )) != -1) {

        switch (option) {

            case 't':
                if (!parse_number(optarg, 1, &threads_num)) {
                    fprintf(stderr, "Invalid threads_num\n");
                    return 1;
                }

                has_threads = true;
                break;

            case 's':
                if (!parse_number(optarg, 0, &seed)) {
                    fprintf(stderr, "Invalid seed\n");
                    return 1;
                }

                has_seed = true;
                break;

            case 'a':
                if (!parse_number(optarg, 1, &array_size)) {
                    fprintf(stderr, "Invalid array_size\n");
                    return 1;
                }

                has_size = true;
                break;

            default:
                return 1;
        }
    }

    /*
     * Проверка обязательных аргументов.
     */
    if (!has_threads || !has_seed ||
        !has_size || optind < argc) {

        fprintf(
            stderr,
            "Usage: %s --threads_num NUM "
            "--seed NUM --array_size NUM\n",
            argv[0]
        );

        return 1;
    }

    if (threads_num > array_size) {
        fprintf(
            stderr,
            "Thread number must not exceed array size\n"
        );

        return 1;
    }

    /*
     * Выделяем память под массив.
     */
    int *array = malloc(
        sizeof(int) * (size_t)array_size
    );

    if (array == NULL) {
        perror("malloc");
        return 1;
    }

    /*
     * Генерируем массив через функцию
     * из лабораторной работы №3.
     *
     * Генерация происходит ДО запуска таймера.
     */
    GenerateArray(array, array_size, seed);

    /*
     * Выделяем память под идентификаторы потоков.
     */
    pthread_t *threads = malloc(
        sizeof(pthread_t) * (size_t)threads_num
    );

    if (threads == NULL) {
        perror("malloc");

        free(array);
        return 1;
    }

    /*
     * Аргументы для каждого потока.
     */
    struct SumArgs *args = calloc(
        (size_t)threads_num,
        sizeof(struct SumArgs)
    );

    if (args == NULL) {
        perror("calloc");

        free(threads);
        free(array);
        return 1;
    }

    /*
     * Запоминаем время начала вычислений.
     */
    struct timespec start_time;
    struct timespec finish_time;

    if (clock_gettime(
        CLOCK_MONOTONIC, &start_time
    ) == -1) {
        perror("clock_gettime");

        free(args);
        free(threads);
        free(array);
        return 1;
    }

    /*
     * Создаём потоки.
     */
    unsigned int created = 0;
    bool failed = false;

    for (unsigned int i = 0; i < threads_num; i++) {

        /*
         * Определяем участок массива для потока.
         */
        args[i].array = array;

        args[i].begin =
            (size_t)i * array_size / threads_num;

        args[i].end =
            (size_t)(i + 1) * array_size / threads_num;

        args[i].result = 0;

        /*
         * Создаём поток.
         */
        int error = pthread_create(
            &threads[i],
            NULL,
            ThreadSum,
            &args[i]
        );

        if (error != 0) {
            fprintf(
                stderr,
                "pthread_create failed: %s\n",
                strerror(error)
            );

            failed = true;
            break;
        }

        created++;
    }

    /*
     * Ожидаем завершения созданных потоков.
     */
    for (unsigned int i = 0; i < created; i++) {

        int error = pthread_join(
            threads[i],
            NULL
        );

        if (error != 0) {
            fprintf(
                stderr,
                "pthread_join failed: %s\n",
                strerror(error)
            );

            failed = true;
        }
    }

    if (failed) {
        free(args);
        free(threads);
        free(array);

        return 1;
    }

    /*
     * Объединяем частичные суммы.
     */
    long long total_sum = 0;

    for (unsigned int i = 0; i < threads_num; i++) {
        total_sum += args[i].result;
    }

    /*
     * Запоминаем время окончания вычислений.
     */
    if (clock_gettime(
        CLOCK_MONOTONIC, &finish_time
    ) == -1) {
        perror("clock_gettime");

        free(args);
        free(threads);
        free(array);

        return 1;
    }

    double elapsed_time = elapsed_ms(
        &start_time,
        &finish_time
    );

    /*
     * Вывод результатов.
     */
    printf("Threads: %u\n", threads_num);
    printf("Array size: %u\n", array_size);
    printf("Seed: %u\n", seed);

    printf("Total: %lld\n", total_sum);
    printf("Elapsed time: %.3f ms\n", elapsed_time);

    /*
     * Освобождаем память.
     */
    free(args);
    free(threads);
    free(array);

    return 0;
}
