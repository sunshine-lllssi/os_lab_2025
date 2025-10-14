#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Использование: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // Дочерний процесс: запускаем sequential_min_max с ПРАВИЛЬНЫМИ аргументами
        char *args[] = {
            "./sequential_min_max",
            "--seed", argv[1],      // seed с двойным дефисом
            "--array_size", argv[2], // array_size с двойным дефисом
            NULL
        };
        execv("./sequential_min_max", args);
        
        // Если execv вернулся — ошибка
        perror("execv failed");
        exit(1);
    } else if (pid > 0) {
        // Родительский процесс: ждём завершения дочернего
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
            printf("Дочерний процесс завершился с кодом %d\n", WEXITSTATUS(status));
        }
    } else {
        // Ошибка fork
        perror("fork failed");
        return 1;
    }

    return 0;
}