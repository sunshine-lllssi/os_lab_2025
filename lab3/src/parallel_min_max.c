#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = -1;
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 0}, // добавляем --timeout
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed должен быть положительным числом\n");
                            return 1;
                        }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size должен быть положительным числом\n");
                            return 1;
                        }
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum должен быть положительным числом\n");
                            return 1;
                        }
                        break;
                    case 3:
                        with_files = true;
                        break;
                    case 4:
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            printf("timeout должен быть положительным числом\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Неизвестный индекс опции %d\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt вернул код 0%o\n", c);
        }
    }

    if (optind < argc) {
        printf("Есть необработанные аргументы\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Использование: %s --seed \"число\" --array_size \"число\" --pnum \"число\" [--timeout \"сек\"] [--by_files]\n", argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    if (!array) {
        printf("Ошибка выделения памяти для массива\n");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    int pipes[2 * pnum];
    char filenames[pnum][50];

    if (!with_files) {
        for (int i = 0; i < pnum; i++) {
            if (pipe(pipes + i * 2) < 0) {
                printf("Ошибка создания pipe\n");
                free(array);
                return 1;
            }
        }
    } else {
        for (int i = 0; i < pnum; i++) {
            snprintf(filenames[i], sizeof(filenames[i]), "min_max_%d.txt", i);
        }
    }

    pid_t *child_pids = malloc(pnum * sizeof(pid_t)); //явное сохранение PID для возможности завершения
    if (!child_pids) {
        printf("Ошибка выделения памяти для PID процессов\n");
        free(array);
        return 1;
    }

    int active_child_processes = 0;
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            if (child_pid == 0) {
                int chunk_size = array_size / pnum;
                int begin = i * chunk_size;
                int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
                struct MinMax local_min_max = GetMinMax(array, begin, end);

                if (with_files) {
                    FILE *file = fopen(filenames[i], "w");
                    if (file == NULL) {
                        printf("Ошибка открытия файла %s\n", filenames[i]);
                        exit(1);
                    }
                    fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
                    fclose(file);
                } else {
                    close(pipes[i * 2]);
                    write(pipes[i * 2 + 1], &local_min_max.min, sizeof(int));
                    write(pipes[i * 2 + 1], &local_min_max.max, sizeof(int));
                    close(pipes[i * 2 + 1]);
                }
                free(array);
                exit(0);
            } else {
                child_pids[active_child_processes++] = child_pid; //сохраняет информацию  для дочерних процессов
            }
        } else {
            printf("Ошибка создания процесса\n");
            free(child_pids);
            free(array);
            return 1;
        }
    }

    time_t start_sec = time(NULL); //возвращает текущее время в секундах,    time_t-тип данных 
    int remaining = active_child_processes; //счетчик не завершенных дочерних процессов
    bool timeout_reached = false; //флаг, который указывает был ли достигнут максимум ожидания

    while (remaining > 0) {
        int status;
        pid_t finished = waitpid(-1, &status, WNOHANG); // неблокирующая проверка завершения процессов
        if (finished > 0) { //если waitpid вернул PID завершенного процесса (> 0)
            remaining--; //уменьшаем счетчик
        }

        if (timeout > 0) {  //если максимальное значение задано
            time_t elapsed = time(NULL) - start_sec; //вычисляем прошедшее время (текущее время - время начала)
            if (elapsed >= timeout) {
                printf("Достигнут таймаут %d секунд. Завершение дочерних процессов.\n", timeout);
                for (int i = 0; i < active_child_processes; i++) { // проходимся по всем дочерним процессам
                    kill(child_pids[i], SIGKILL); // завершаем немедленно
                }
                timeout_reached = true; // устанавливаем флаг, что максимальное значение достигнуто
                break; 
            }
        }

        usleep(10000); //приостанавливаем выполнение на 10000 миеросекунд, чтобы не нагружать проверкоц постоянной
    }

    if (!timeout_reached) { 
        for (int i = 0; i < active_child_processes; i++) {
            if (child_pids[i] > 0) {
                waitpid(child_pids[i], NULL, 0);//ожидание завершения конкретного процесса
            }
        }
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    if (!timeout_reached) { //было ли максимальное значение
        for (int i = 0; i < pnum; i++) {
            int min = INT_MAX;
            int max = INT_MIN;

            if (with_files) {
                FILE *file = fopen(filenames[i], "r");
                if (file == NULL) {
                    printf("Ошибка открытия файла %s\n", filenames[i]);
                    free(child_pids);
                    free(array);
                    return 1;
                }
                fscanf(file, "%d %d", &min, &max);
                fclose(file);
                remove(filenames[i]);
            } else {
                close(pipes[i * 2 + 1]);
                read(pipes[i * 2], &min, sizeof(int));
                read(pipes[i * 2], &max, sizeof(int));
                close(pipes[i * 2]);
            }

            if (min < min_max.min) min_max.min = min;
            if (max > min_max.max) min_max.max = max;
        }
    } else { //при максимальном значении результаты недоступны
        min_max.min = 0;
        min_max.max = 0;
        printf("Результаты недоступны (превышен таймаут)\n");
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);

    if (!timeout_reached) {
        printf("Минимум: %d\n", min_max.min);
        printf("Максимум: %d\n", min_max.max);
    }
    printf("Затраченное время: %fмс\n", elapsed_time);
    
    if (timeout_reached) { //было макс значение
        return 1;
    } else {
        return 0;
    }
}