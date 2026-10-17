#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Специальные символы компоновщика.
 *
 * etext — конец области машинного кода.
 * edata — конец области инициализированных данных.
 * end   — конец области неинициализированных данных.
 */
extern char etext, edata, end;

/*
 * Макрос для вывода адреса объекта.
 */
#define SHW_ADR(ID, I) \
    printf("ID %-8s is at virtual address: %p\n", \
           ID, (void *)&(I))

/*
 * Глобальные переменные.
 */
const char *cptr =
    "This message is output by the function showit()\n";

char buffer1[25];

/*
 * Объявление функции.
 */
void showit(const char *p);

int main(void) {

    /*
     * Локальная переменная.
     */
    int i = 0;

    printf("\n=== MEMORY SEGMENT BOUNDARIES ===\n");

    printf("Address etext: %p\n", (void *)&etext);
    printf("Address edata: %p\n", (void *)&edata);
    printf("Address end:   %p\n", (void *)&end);

    /*
     * Адреса функций и переменных.
     */
    printf("\n=== VARIABLE AND FUNCTION ADDRESSES ===\n");

    SHW_ADR("main", main);
    SHW_ADR("showit", showit);
    SHW_ADR("cptr", cptr);
    SHW_ADR("buffer1", buffer1);
    SHW_ADR("i", i);

    /*
     * Работа с глобальным массивом.
     */
    strcpy(buffer1, "A demonstration\n");

    printf("\n=== WRITE SYSTEM CALL ===\n");

    fflush(stdout);
    write(STDOUT_FILENO, buffer1, strlen(buffer1));

    /*
     * Вызов функции, использующей malloc().
     */
    showit(cptr);

    return 0;
}

/*
 * Демонстрация динамической памяти.
 */
void showit(const char *p) {

    char *buffer2;

    printf("\n=== DYNAMIC MEMORY ===\n");

    /*
     * Адрес локального указателя buffer2.
     */
    SHW_ADR("buffer2", buffer2);

    /*
     * Выделение динамической памяти.
     */
    buffer2 = malloc(strlen(p) + 1);

    if (buffer2 != NULL) {

        printf(
            "Allocated memory at: %p\n",
            (void *)buffer2
        );

        strcpy(buffer2, p);

        printf("%s", buffer2);

        /*
         * Освобождение динамической памяти.
         */
        free(buffer2);

        printf("Memory successfully released.\n");

    } else {

        printf("Memory allocation error\n");
        exit(1);
    }
}
