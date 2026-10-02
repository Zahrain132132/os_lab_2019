#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  /*
   * Обработка аргументов командной строки.
   */
  while (true) {
    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);

            if (seed < 0) {
              printf("Seed must be non-negative\n");
              return 1;
            }
            break;

          case 1:
            array_size = atoi(optarg);

            if (array_size <= 0) {
              printf("Array size must be positive\n");
              return 1;
            }
            break;

          case 2:
            pnum = atoi(optarg);

            if (pnum <= 0) {
              printf("Process number must be positive\n");
              return 1;
            }
            break;

          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;

      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  /*
   * Проверяем корректность аргументов.
   */
  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf(
        "Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" "
        "[--by_files]\n",
        argv[0]
    );
    return 1;
  }

  if (pnum > array_size) {
    printf("Process number must not exceed array size\n");
    return 1;
  }

  /*
   * Создаём и заполняем массив.
   */
  int *array = malloc(sizeof(int) * array_size);

  if (array == NULL) {
    printf("Memory allocation failed\n");
    return 1;
  }

  GenerateArray(array, array_size, seed);

  /*
   * Для режима pipe каждому дочернему процессу
   * создаём собственный канал.
   *
   * pipes[i][0] - чтение
   * pipes[i][1] - запись
   */
  int (*pipes)[2] = NULL;

  if (!with_files) {
    pipes = malloc(sizeof(int[2]) * pnum);

    if (pipes == NULL) {
      printf("Memory allocation failed\n");
      free(array);
      return 1;
    }

    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        printf("Pipe creation failed\n");
        free(pipes);
        free(array);
        return 1;
      }
    }
  }

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  /*
   * Создаём дочерние процессы.
   */
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();

    if (child_pid < 0) {
      printf("Fork failed!\n");

      if (pipes != NULL) {
        free(pipes);
      }

      free(array);
      return 1;
    }

    if (child_pid == 0) {
      /*
       * CHILD PROCESS
       *
       * Вычисляем диапазон массива,
       * принадлежащий этому процессу.
       */
      int begin = i * array_size / pnum;
      int end = (i + 1) * array_size / pnum;

      struct MinMax local_min_max =
          GetMinMax(array, begin, end);

      if (with_files) {
        /*
         * Вариант 1:
         * передаём результат через файл.
         */
        char filename[64];

        sprintf(filename, "result_%d.txt", i);

        FILE *file = fopen(filename, "w");

        if (file == NULL) {
          printf("Cannot open file %s\n", filename);
          free(array);
          return 1;
        }

        fprintf(
            file,
            "%d %d\n",
            local_min_max.min,
            local_min_max.max
        );

        fclose(file);

      } else {
        /*
         * Вариант 2:
         * передаём структуру MinMax через pipe.
         *
         * Ребёнку нужен только конец pipe
         * для записи.
         */
        close(pipes[i][0]);

        ssize_t bytes_written = write(
            pipes[i][1],
            &local_min_max,
            sizeof(struct MinMax)
        );

        if (bytes_written != sizeof(struct MinMax)) {
          printf("Pipe write failed\n");
          close(pipes[i][1]);
          free(pipes);
          free(array);
          return 1;
        }

        close(pipes[i][1]);
      }

      /*
       * После выполнения работы дочерний процесс
       * завершается и не продолжает цикл fork.
       */
      if (pipes != NULL) {
        free(pipes);
      }

      free(array);
      return 0;
    }

    /*
     * Эту строку выполняет только parent.
     */
    active_child_processes += 1;
  }

  /*
   * PARENT PROCESS
   *
   * В режиме pipe родителю не нужны концы
   * каналов, предназначенные для записи.
   */
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  /*
   * Ждём завершения всех дочерних процессов.
   */
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  /*
   * Начальные значения глобального результата.
   */
  struct MinMax min_max;

  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  /*
   * Собираем результаты всех дочерних процессов.
   */
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      /*
       * Читаем локальный результат из файла.
       */
      char filename[64];

      sprintf(filename, "result_%d.txt", i);

      FILE *file = fopen(filename, "r");

      if (file == NULL) {
        printf("Cannot open file %s\n", filename);
        free(array);
        return 1;
      }

      if (fscanf(file, "%d %d", &min, &max) != 2) {
        printf("Cannot read data from file %s\n", filename);
        fclose(file);
        free(array);
        return 1;
      }

      fclose(file);

      /*
       * После чтения временный файл удаляем.
       */
      remove(filename);

    } else {
      /*
       * Получаем структуру MinMax непосредственно
       * через pipe.
       */
      struct MinMax local_min_max;

      ssize_t bytes_read = read(
          pipes[i][0],
          &local_min_max,
          sizeof(struct MinMax)
      );

      if (bytes_read != sizeof(struct MinMax)) {
        printf("Pipe read failed\n");
        close(pipes[i][0]);
        free(pipes);
        free(array);
        return 1;
      }

      close(pipes[i][0]);

      min = local_min_max.min;
      max = local_min_max.max;
    }

    /*
     * Объединяем локальный результат
     * с глобальным.
     */
    if (min < min_max.min) {
      min_max.min = min;
    }

    if (max > min_max.max) {
      min_max.max = max;
    }
  }

  /*
   * Засекаем время окончания работы.
   */
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time =
      (finish_time.tv_sec - start_time.tv_sec) * 1000.0;

  elapsed_time +=
      (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  /*
   * Освобождаем выделенную память.
   */
  if (pipes != NULL) {
    free(pipes);
  }

  free(array);

  /*
   * Выводим итоговый результат.
   */
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);

  fflush(NULL);

  return 0;
}
