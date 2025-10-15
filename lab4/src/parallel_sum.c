#include "sum_lib.h" 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
extern void GenerateArray(int *array, unsigned int array_size, unsigned int seed); // объявление внешней функции для генерации массива


void *ThreadSum(void *args) { // для потоков
  struct SumArgs *sum_args = (struct SumArgs *)args; //приводим к нужному типу 
  int result = Sum(sum_args); //сумма своей части массива
  return (void *)(intptr_t)result; 
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0; //кол-во потоков
  uint32_t array_size = 0;
  uint32_t seed = 0; //для генирациий случайных чисел


  for (int i = 1; i < argc; i += 2) { //процесс обработки 
  // параметров командной строки, когда программа ожидает аргументы в виде пар "ключ-значение"
        if (i + 1 >= argc) goto usage;
        if (strcmp(argv[i], "--threads_num") == 0) {
            threads_num = (uint32_t)atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--seed") == 0) {
            seed = (uint32_t)atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--array_size") == 0) {
            array_size = (uint32_t)atoi(argv[i + 1]);
        } else {
            goto usage;
        }
    }

    if (!threads_num || !array_size) { //проверка обязательных аргументов
    usage:
        fprintf(stderr, "Usage: %s --threads_num N --seed S --array_size M\n", argv[0]);
        return 1;
    }
  int *array = malloc(sizeof(int) * array_size); //выделение памяти под массив
  if (!array) {
      perror("malloc array"); // ошибка выделения памяти
      return 1;
    }
  GenerateArray(array, array_size, seed);//заполнение массива случайными числами
  struct SumArgs *args = malloc(sizeof(struct SumArgs) * threads_num);
    if (!args) { 
    perror("malloc args");
    free(array);
    return 1;
    }   
    uint32_t chunk = array_size / threads_num;
    for (uint32_t i = 0; i < threads_num; ++i) {
        args[i].array = array; //все пооки работают с одним массивом
        args[i].begin = i * chunk; //начало диапазона
        args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk;//последний поток обрабатывается до конца массиива
    }
  //замер времени и создание потоков
  struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);//время начала

    pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
    if (!threads) {
    perror("malloc threads");
    free(args);
    free(array);
    return 1;
    }
    for (uint32_t i = 0; i < threads_num; ++i) { //создание потоков
        if (pthread_create(&threads[i], NULL, ThreadSum, &args[i])) {
            fprintf(stderr, "pthread_create failed\n");
            free(threads);
            free(args);
            free(array);
            return 1;
        }
    }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    void *result;
    pthread_join(threads[i], &result); //ждем завершение потока и получаемрезультат
    total_sum += (int)(intptr_t)result; //сумируем результаты всех потоков
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  double time_sec = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; //вычисления времени выполнения в сек
  free(threads); //освобождаем память 
  free(args);
  free(array);
  
  printf("Total: %d\n", total_sum); //общая сумма 
  printf("Time: %.6f seconds\n", time_sec);
  return 0;
}
