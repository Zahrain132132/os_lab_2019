#include "sum.h"

/*
 * Вычисление суммы элементов массива.
 */
long long Sum(const struct SumArgs *args) {

    long long sum = 0;

    for (size_t i = args->begin; i < args->end; i++) {
        sum += args->array[i];
    }

    return sum;
}