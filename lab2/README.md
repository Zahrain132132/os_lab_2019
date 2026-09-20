# Лабораторная работа 2

Решение четырёх заданий из `text/lab2.md`. Среда выполнения: Linux (GitHub Codespaces), GCC, GNU Make и CUnit.

## Сборка и проверка

```bash
sudo apt-get update
sudo apt-get install -y build-essential libcunit1 libcunit1-dev libcunit1-doc
cd lab2
make
make check
```

Все результаты сборки находятся в `build/`, который исключён из Git.

## Задание 1

```bash
make build/swap
./build/swap
```

Результат: `b a`. Функция `Swap` получает адреса двух изменяемых символов и обменивает значения через временную переменную.

## Задание 2

```bash
make build/revert_direct
./build/revert_direct Hello
./build/revert_direct "String with spaces"
./build/revert_direct ""
```

`RevertString` разворачивает байты строки на месте за O(n) времени и O(1) дополнительной памяти; нулевой терминатор остаётся на месте. Контракт: ненулевой указатель на изменяемую строку с завершающим `\0`. Алгоритм рассчитан на однобайтовые символы, не на Unicode в UTF-8.

`main` принимает ровно один аргумент, выделяет `strlen(argv[1]) + 1` байт, проверяет `malloc`, копирует строку, вызывает функцию, выводит результат и освобождает память.

## Задание 3

```bash
make build/revert_static build/revert_dynamic
./build/revert_static Hello
export LD_LIBRARY_PATH="$PWD/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
./build/revert_dynamic Hello
ar t build/librevert.a
readelf -d build/revert_static | grep NEEDED
readelf -d build/revert_dynamic | grep NEEDED
```

`revert_static` явно линкуется с `build/librevert.a`, чтобы не выбрать `.so` из того же каталога. Статическая здесь только библиотека RevertString; системная libc может оставаться динамической. `revert_dynamic` использует `build/librevert.so` через `-Lbuild -lrevert`.

## Задание 4

```bash
make build/tests
export LD_LIBRARY_PATH="$PWD/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
./build/tests
ldd build/revert_dynamic | grep librevert
ldd build/tests | grep librevert
```

Приложение и CUnit используют один и тот же файл `build/librevert.so`. Исходные четыре проверки сохранены. Код завершения тестовой программы учитывает не только ошибку инфраструктуры CUnit, но и провал утверждений.

`make check` дополнительно проверяет пустую строку, один символ, чётную и нечётную длину, пробелы и неправильное число аргументов для всех трёх вариантов программы. Также проверяется совпадение путей загруженной библиотеки.

## Основные флаги

- `-I`: каталог заголовков; `-L`: каталог библиотек при линковке.
- `-lrevert`: поиск `librevert.so` или `librevert.a`.
- `-c`: объектный файл без линковки; `-o`: имя результата.
- `-fPIC`: позиционно независимый код; `-shared`: общая библиотека.
- `ar rcs`: создание или обновление архива и индекса символов.
- `LD_LIBRARY_PATH`: дополнительные каталоги поиска `.so` при запуске, не замена `-L`.

Удаление только результатов сборки: `make clean`.
