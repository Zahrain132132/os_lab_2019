
#include <stdio.h>
#include <pthread.h>

// Два независимых мьютекса
pthread_mutex_t mutex_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_b = PTHREAD_MUTEX_INITIALIZER;

// Барьер для синхронизации начала ожидания
pthread_barrier_t barrier;

// Первый поток
void *thread_one(void *arg) {
    (void)arg;

    pthread_mutex_lock(&mutex_a);
    printf("Thread 1: locked mutex A\n");

    pthread_barrier_wait(&barrier);

    printf("Thread 1: waiting for mutex B...\n");
    pthread_mutex_lock(&mutex_b);

    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    return NULL;
}

// Второй поток
void *thread_two(void *arg) {
    (void)arg;

    pthread_mutex_lock(&mutex_b);
    printf("Thread 2: locked mutex B\n");

    pthread_barrier_wait(&barrier);

    printf("Thread 2: waiting for mutex A...\n");
    pthread_mutex_lock(&mutex_a);

    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);

    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    setbuf(stdout, NULL);

    pthread_barrier_init(&barrier, NULL, 2);

    pthread_create(&t1, NULL, thread_one, NULL);
    pthread_create(&t2, NULL, thread_two, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);

    return 0;
}
