#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>

// глобальные переменные
long long global_result = 1; // глобальный результат факториала
long long mod_value = 1;  // значение модуля для операции %
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER; // мьютекс для защиты global_result

struct ThreadArgs { //стуктура для передачи аргументов потокам
    long long start;
    long long end;
};

void *compute_partial_factorial(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg; //приведение к структуре
    long long local_product = 1; // локальная переменная для частичного произведения

    for (long long i = args->start; i <= args->end; ++i) { //вычисление произведения чисел 
        local_product = (local_product * i) % mod_value; //умножаем и сразу берем по модулю
    }

    // обновляем глобальный результат под мьютексом
    pthread_mutex_lock(&result_mutex); //берем мьютекс
    global_result = (global_result * local_product) % mod_value; //обновляем результат
    pthread_mutex_unlock(&result_mutex); //освобождаем мьютекс

    return NULL;
}

int main(int argc, char *argv[]) {
    long long k = -1; //число для факториала
    long long pnum = -1; //кол-во потоков 
    mod_value = -1; //модуль

    // разбор аргументов(разбиваем на части аргументы командной строки)
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = atoll(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoll(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod_value = atoll(argv[i] + 6);
        }
    }

    // проверка аргументов
    if (k < 0 || pnum <= 0 || mod_value <= 0) {
        fprintf(stderr, "Usage: %s -k <num> --pnum=<num> --mod=<num>\n", argv[0]);
        return 1; //ошибка
    }

    if (k == 0 || k == 1) {
        printf("%lld\n", 1 % mod_value);
        return 0;
    }

    // ограничиваем число потоков
    if (pnum > k) pnum = k; // нельзя иметь потоков больше чем чисел

    // выделяем аргументы для потоков
    struct ThreadArgs *thread_args = malloc(pnum * sizeof(struct ThreadArgs)); //память для аргументов
    pthread_t *threads = malloc(pnum * sizeof(pthread_t)); //память для идентификаторов потоков

    // распределяем диапазон [1, k] между потоками
    long long chunk_size = k / pnum; //базовый размер диапазона для потока
    long long remainder = k % pnum; //остаток для равномерного распределения
    long long current = 1;      // текущее число для распределения

    for (long long i = 0; i < pnum; ++i) {
        thread_args[i].start = current;
        thread_args[i].end = current + chunk_size - 1;
        if (i < remainder) {
            thread_args[i].end++;  // распределяем остаток
        }
        if (thread_args[i].end > k) {
            thread_args[i].end = k;
        }
        current = thread_args[i].end + 1; //следующее начао диапазона
    }

    // создаём потоки
    for (long long i = 0; i < pnum; ++i) {
        if (pthread_create(&threads[i], NULL, compute_partial_factorial, &thread_args[i]) != 0) {
            perror("pthread_create"); //ошибка создания потока
            free(thread_args);
            free(threads);
            return 1;
        }
    }

    // ждём завершения каждого потока
    for (long long i = 0; i < pnum; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("%lld\n", global_result);

    free(thread_args);
    free(threads);
    return 0;
}