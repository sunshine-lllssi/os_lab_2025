#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    pid_t pid;
    int i;

    // создаём 5 дочерних процессов
    for (i = 0; i < 5; i++) {
        pid = fork();
        if (pid == 0) {
            // дочерний процесс
            printf("Дочерний процесс %d (PID: %d) создан. Завершаю работу.\n", i, getpid());
            _exit(0);  // завершаемся немедленно
        }
        // родитель продолжает цикл
    }

    sleep(30);  // за это время дети станут зомби

    for (i = 0; i < 5; i++) {
        wait(NULL);  // очистка зомби
    }

    printf("Все зомби-процессы собраны. Родительский процесс завершает работу.\n");
    return 0;
}
