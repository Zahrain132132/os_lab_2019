CC = gcc
AR = ar

CFLAGS = -std=c11 -Wall -Wextra -O2

.PHONY: all clean

# Сборка всех программ лабораторной работы
all: parallel_min_max process_memory zombie_demo psum

# Задание 1: параллельный поиск min/max
parallel_min_max: parallel_min_max.c find_min_max.c utils.c find_min_max.h utils.h
	$(CC) $(CFLAGS) parallel_min_max.c find_min_max.c utils.c -o $@

# Задание 2: демонстрация зомби-процессов
zombie_demo: zombie_demo.c
	$(CC) $(CFLAGS) zombie_demo.c -o $@

# Задание 3: виртуальная память
process_memory: process_memory.c
	$(CC) -std=c11 -Wall -Wextra -O0 process_memory.c -o $@

# Задание 5: компиляция функции суммирования
sum.o: sum.c sum.h
	$(CC) $(CFLAGS) -c sum.c -o sum.o

# Создание статической библиотеки
libsum.a: sum.o
	$(AR) rcs libsum.a sum.o

# Задание 6: многопоточная сумма
psum: parallel_sum.c utils.c utils.h sum.h libsum.a
	$(CC) $(CFLAGS) -pthread parallel_sum.c utils.c ./libsum.a -o $@

# Очистка результатов компиляции
clean:
	rm -f parallel_min_max process_memory zombie_demo psum *.o *.a