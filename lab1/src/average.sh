#!/bin/bash

count=$#
sum=0

for number in "$@"
do
    sum=$((sum + number))
done

if [ "$count" -eq 0 ]; then
    echo "Количество чисел: 0"
    echo "Среднее арифметическое: невозможно вычислить"
else
    average=$((sum / count))
    echo "Количество чисел: $count"
    echo "Среднее арифметическое: $average"
fi
