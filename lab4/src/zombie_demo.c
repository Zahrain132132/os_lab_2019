#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    /*
     * Дочерний процесс.
     */
    if (pid == 0) {
        printf("Child process: PID = %d\n", getpid());
        fflush(stdout);

        /*
         * Ребёнок завершается.
         */
        _exit(0);
    }

    /*
     * Родительский процесс.
     */
    printf("Parent process: PID = %d\n", getpid());
    printf("Child PID = %d\n", pid);

    printf("\nChild is now becoming a zombie...\n");
    printf("Parent will NOT call waitpid() for 12 seconds.\n");

    fflush(stdout);

    /*
     * Родитель не забирает статус ребёнка.
     * Дочерний процесс становится зомби.
     */
    sleep(12);

    /*
     * Родитель получает статус завершения ребёнка.
     * Зомби исчезает.
     */
    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    printf("\nwaitpid() called successfully!\n");
    printf("Zombie process has been removed.\n");

    fflush(stdout);

    /*
     * Оставляем родителя работающим,
     * чтобы проверить отсутствие зомби.
     */
    sleep(12);

    printf("\nParent process finished.\n");

    return 0;
}