#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

/*
 * Флаг срабатывания таймера.
 * sig_atomic_t безопасно изменять в обработчике сигнала.
 */
static volatile sig_atomic_t timeout_expired = 0;

/*
 * Обработчик SIGALRM.
 */
static void alarm_handler(int signum) {
    (void)signum;
    timeout_expired = 1;
}

/*
 * Проверка числовых аргументов.
 */
static bool parse_number(const char *text, int minimum, int *result) {
    char *end = NULL;
    errno = 0;

    long value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' ||
        value < minimum || value > INT_MAX) {
        return false;
    }

    *result = (int)value;
    return true;
}

/*
 * Удаление временных файлов с результатами.
 */
static void remove_result_files(int pnum) {
    for (int i = 0; i < pnum; i++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "result_%d.txt", i);
        remove(filename);
    }
}

int main(int argc, char **argv) {

    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0;

    bool with_files = false;

    /*
     * Обработка аргументов командной строки.
     */
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {"timeout", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    while (true) {
        int option_index = 0;

        int c = getopt_long(
            argc, argv, "f", options, &option_index
        );

        if (c == -1) {
            break;
        }

        if (c == 'f') {
            with_files = true;
            continue;
        }

        if (c == '?') {
            return 1;
        }

        if (c == 0) {
            switch (option_index) {
                case 0:
                    if (!parse_number(optarg, 0, &seed)) {
                        fprintf(stderr, "Invalid seed\n");
                        return 1;
                    }
                    break;

                case 1:
                    if (!parse_number(optarg, 1, &array_size)) {
                        fprintf(stderr, "Invalid array size\n");
                        return 1;
                    }
                    break;

                case 2:
                    if (!parse_number(optarg, 1, &pnum)) {
                        fprintf(stderr, "Invalid process number\n");
                        return 1;
                    }
                    break;

                case 4:
                    if (!parse_number(optarg, 1, &timeout)) {
                        fprintf(stderr, "Timeout must be positive\n");
                        return 1;
                    }
                    break;

                default:
                    fprintf(stderr, "Unknown option\n");
                    return 1;
            }
        }
    }

    /*
     * Проверка аргументов.
     */
    if (optind < argc ||
        seed == -1 ||
        array_size == -1 ||
        pnum == -1) {

        printf(
            "Usage: %s --seed NUM --array_size NUM "
            "--pnum NUM [--by_files] [--timeout NUM]\n",
            argv[0]
        );

        return 1;
    }

    if (pnum > array_size) {
        fprintf(stderr,
                "Process number must not exceed array size\n");
        return 1;
    }

    /*
     * Создаём массив.
     */
    int *array = malloc(sizeof(int) * (size_t)array_size);

    if (array == NULL) {
        perror("malloc");
        return 1;
    }

    GenerateArray(array, array_size, seed);

    /*
     * Массив идентификаторов дочерних процессов.
     */
    pid_t *children = calloc((size_t)pnum, sizeof(pid_t));

    if (children == NULL) {
        perror("calloc");
        free(array);
        return 1;
    }

    /*
     * Создаём каналы pipe, если не используется
     * режим передачи результатов через файлы.
     */
    int (*pipes)[2] = NULL;
    int pipes_created = 0;

    if (!with_files) {
        pipes = malloc(sizeof(int[2]) * (size_t)pnum);

        if (pipes == NULL) {
            perror("malloc");
            free(children);
            free(array);
            return 1;
        }

        for (int i = 0; i < pnum; i++) {
            pipes[i][0] = -1;
            pipes[i][1] = -1;
        }

        for (int i = 0; i < pnum; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");

                for (int j = 0; j < pipes_created; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                free(pipes);
                free(children);
                free(array);
                return 1;
            }

            pipes_created++;
        }
    }

    int exit_code = 1;
    int created = 0;
    bool child_failed = false;

    struct timeval start_time;
    struct timeval finish_time;

    gettimeofday(&start_time, NULL);

    /*
     * ЗАДАНИЕ 1:
     * Устанавливаем обработчик SIGALRM.
     */
    if (timeout > 0) {
        if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
            perror("signal");
            goto cleanup;
        }

        /*
         * Запускаем таймер.
         * По истечении timeout секунд поступит SIGALRM.
         */
        alarm((unsigned int)timeout);

        printf("Timeout enabled: %d seconds\n", timeout);
        fflush(stdout);
    }

    /*
     * Создание дочерних процессов.
     */
    for (int i = 0; i < pnum; i++) {

        if (timeout_expired) {
            break;
        }

        pid_t child_pid = fork();

        if (child_pid < 0) {
            perror("fork");
            child_failed = true;
            break;
        }

        if (child_pid == 0) {

            /*
             * CHILD PROCESS
             *
             * Дочернему процессу собственный
             * обработчик таймера не нужен.
             */
            if (timeout > 0) {
                signal(SIGALRM, SIG_DFL);
                alarm(0);
            }

            /*
             * Закрываем ненужные дескрипторы.
             * Ребёнку нужен только его канал записи.
             */
            if (!with_files) {
                for (int j = 0; j < pnum; j++) {
                    close(pipes[j][0]);

                    if (j != i) {
                        close(pipes[j][1]);
                    }
                }
            }

            /*
             * Вычисляем участок массива.
             */
            int begin =
                (int)((long long)i * array_size / pnum);

            int end =
                (int)((long long)(i + 1) * array_size / pnum);

            struct MinMax local_min_max =
                GetMinMax(array, begin, end);

            if (with_files) {

                /*
                 * Передача результата через файл.
                 */
                char filename[64];

                snprintf(
                    filename,
                    sizeof(filename),
                    "result_%d.txt",
                    i
                );

                FILE *file = fopen(filename, "w");

                if (file == NULL) {
                    perror("fopen");
                    _exit(1);
                }

                int written = fprintf(
                    file,
                    "%d %d\n",
                    local_min_max.min,
                    local_min_max.max
                );

                if (written < 0 || fclose(file) != 0) {
                    fprintf(stderr, "File write failed\n");
                    _exit(1);
                }

            } else {

                /*
                 * Передача результата через pipe.
                 */
                ssize_t bytes_written;

                do {
                    bytes_written = write(
                        pipes[i][1],
                        &local_min_max,
                        sizeof(struct MinMax)
                    );
                } while (
                    bytes_written == -1 && errno == EINTR
                );

                if (bytes_written != sizeof(struct MinMax)) {
                    fprintf(stderr, "Pipe write failed\n");
                    _exit(1);
                }

                close(pipes[i][1]);
            }

            /*
             * Завершаем дочерний процесс.
             */
            _exit(0);
        }

        /*
         * PARENT PROCESS
         *
         * Сохраняем PID созданного ребёнка.
         */
        children[created] = child_pid;
        created++;
    }

    /*
     * Родителю не нужны концы pipe для записи.
     */
    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            close(pipes[i][1]);
            pipes[i][1] = -1;
        }
    }

    if (child_failed) {
        goto cleanup;
    }

    /*
     * Ожидание дочерних процессов.
     */
    int remaining = created;

    if (timeout == 0) {

        /*
         * Если таймаут не указан:
         * обычное блокирующее ожидание.
         */
        for (int i = 0; i < created; i++) {
            int status;
            pid_t result;

            do {
                result = waitpid(children[i], &status, 0);
            } while (result == -1 && errno == EINTR);

            if (result == -1) {
                perror("waitpid");
                child_failed = true;
                break;
            }

            children[i] = 0;
            remaining--;

            if (!WIFEXITED(status) ||
                WEXITSTATUS(status) != 0) {
                child_failed = true;
            }
        }

    } else {

        /*
         * Если таймаут указан:
         * неблокирующее ожидание с WNOHANG.
         */
        while (remaining > 0 && !timeout_expired) {

            for (int i = 0; i < created; i++) {

                if (children[i] == 0) {
                    continue;
                }

                int status;

                pid_t result = waitpid(
                    children[i],
                    &status,
                    WNOHANG
                );

                if (result == children[i]) {

                    children[i] = 0;
                    remaining--;

                    if (!WIFEXITED(status) ||
                        WEXITSTATUS(status) != 0) {
                        child_failed = true;
                    }

                } else if (result == -1 && errno != EINTR) {
                    perror("waitpid");
                    child_failed = true;
                    break;
                }
            }

            if (child_failed) {
                break;
            }

            /*
             * Небольшая пауза между проверками.
             * Она не блокирует обработку SIGALRM.
             */
            if (remaining > 0 && !timeout_expired) {
                struct timespec pause_time = {
                    .tv_sec = 0,
                    .tv_nsec = 10000000L
                };

                nanosleep(&pause_time, NULL);
            }
        }
    }

    /*
     * Отключаем таймер.
     */
    if (timeout > 0) {
        alarm(0);
    }

    /*
     * Обработка истечения времени.
     */
    if (timeout_expired) {
        fprintf(
            stderr,
            "\nTimeout expired after %d seconds!\n",
            timeout
        );

        fprintf(
            stderr,
            "Terminating unfinished child processes...\n"
        );

        goto cleanup;
    }

    if (child_failed) {
        fprintf(stderr, "Child process failed\n");
        goto cleanup;
    }

    /*
     * Объединение результатов.
     */
    struct MinMax min_max;

    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {

        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {

            /*
             * Получаем результат из файла.
             */
            char filename[64];

            snprintf(
                filename,
                sizeof(filename),
                "result_%d.txt",
                i
            );

            FILE *file = fopen(filename, "r");

            if (file == NULL) {
                perror("fopen");
                goto cleanup;
            }

            if (fscanf(file, "%d %d", &min, &max) != 2) {
                fprintf(
                    stderr,
                    "Cannot read data from %s\n",
                    filename
                );

                fclose(file);
                goto cleanup;
            }

            fclose(file);
            remove(filename);

        } else {

            /*
             * Получаем результат через pipe.
             */
            struct MinMax local_min_max;
            ssize_t bytes_read;

            do {
                bytes_read = read(
                    pipes[i][0],
                    &local_min_max,
                    sizeof(struct MinMax)
                );
            } while (
                bytes_read == -1 && errno == EINTR
            );

            if (bytes_read != sizeof(struct MinMax)) {
                fprintf(stderr, "Pipe read failed\n");
                goto cleanup;
            }

            close(pipes[i][0]);
            pipes[i][0] = -1;

            min = local_min_max.min;
            max = local_min_max.max;
        }

        if (min < min_max.min) {
            min_max.min = min;
        }

        if (max > min_max.max) {
            min_max.max = max;
        }
    }

    /*
     * Замер времени выполнения.
     */
    gettimeofday(&finish_time, NULL);

    double elapsed_time =
        (finish_time.tv_sec - start_time.tv_sec) * 1000.0;

    elapsed_time +=
        (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);

    exit_code = 0;

cleanup:

    /*
     * Отключаем таймер при любом завершении.
     */
    if (timeout > 0) {
        alarm(0);
    }

    /*
     * Принудительно завершаем оставшихся детей.
     */
    for (int i = 0; i < created; i++) {

        if (children[i] > 0) {
            if (exit_code != 0) {

                printf(
                    "Sending SIGKILL to PID %ld\n",
                    (long)children[i]
                );

                if (kill(children[i], SIGKILL) == -1 &&
                    errno != ESRCH) {
                    perror("kill");
                }
            }

            /*
             * Получаем статус завершения,
             * чтобы не оставлять зомби.
             */
            pid_t result;

            do {
                result = waitpid(children[i], NULL, 0);
            } while (result == -1 && errno == EINTR);

            if (result == -1 && errno != ECHILD) {
                perror("waitpid");
            }
        }
    }

    /*
     * Закрываем оставшиеся каналы.
     */
    if (pipes != NULL) {
        for (int i = 0; i < pipes_created; i++) {
            if (pipes[i][0] >= 0) {
                close(pipes[i][0]);
            }

            if (pipes[i][1] >= 0) {
                close(pipes[i][1]);
            }
        }

        free(pipes);
    }

    /*
     * Удаляем временные файлы.
     */
    if (with_files) {
        remove_result_files(pnum);
    }

    free(children);
    free(array);

    return exit_code;
}
