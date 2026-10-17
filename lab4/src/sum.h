#ifndef SUM_H
#define SUM_H

#include <stddef.h>

/*
 * Аргументы для вычисления частичной суммы.
 */
struct SumArgs {
    const int *array;
    size_t begin;
    size_t end;
    long long result;
};

/*
 * Вычисление суммы элементов массива
 * на участке [begin, end).
 */
long long Sum(const struct SumArgs *args);

#endif