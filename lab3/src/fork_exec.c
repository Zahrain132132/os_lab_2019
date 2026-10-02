#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
  printf("Parent process started. PID: %d\n", getpid());

  pid_t child_pid = fork();

  if (child_pid < 0) {
    printf("Fork failed!\n");
    return 1;
  }

  if (child_pid == 0) {
    /*
     * Этот код выполняется дочерним процессом.
     */
    printf(
        "Child process started. PID: %d, Parent PID: %d\n",
        getpid(),
        getppid()
    );

    printf("Child starts sequential_min_max using exec...\n");

    /*
     * exec заменяет текущую программу дочернего процесса
     * программой sequential_min_max.
     *
     * Аргументы:
     *   argv[0] = имя программы
     *   argv[1] = seed
     *   argv[2] = array_size
     *   NULL    = конец списка аргументов
     */
    execl(
        "./sequential_min_max",
        "sequential_min_max",
        "123",
        "100",
        (char *)NULL
    );

    /*
     * Если exec сработал успешно, выполнение сюда
     * НИКОГДА не дойдёт.
     */
    perror("exec failed");
    return 1;
  }

  /*
   * Сюда попадает родительский процесс.
   */
  printf(
      "Parent created child with PID: %d\n",
      child_pid
  );

  /*
   * Родитель ждёт завершения дочернего процесса.
   */
  int status;
  waitpid(child_pid, &status, 0);

  if (WIFEXITED(status)) {
    printf(
        "Child finished with exit code: %d\n",
        WEXITSTATUS(status)
    );
  }

  printf("Parent process finished.\n");

  return 0;
}