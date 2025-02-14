#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#define MAX_STR_LEN 1024

void check_error(int result, const char *msg) {
    if (result == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void check_file_error(int fd, const char *filename) {
    if (fd == -1) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }
}

int main() {
    int pipe1[2], pipe2[2];
    pid_t pid1, pid2;
    char filename1[MAX_STR_LEN], filename2[MAX_STR_LEN];
    char buffer[MAX_STR_LEN];
    int fd1, fd2;
    ssize_t num_bytes;

    check_error(pipe(pipe1), "pipe1 creation failed");
    check_error(pipe(pipe2), "pipe2 creation failed");

    printf("Введите имя файла для child1: ");
    if (fgets(filename1, MAX_STR_LEN, stdin) == NULL) {
        perror("fgets failed");
        exit(EXIT_FAILURE);
    }
    filename1[strcspn(filename1, "\n")] = '\0';  // Удаляем символ новой строки

    printf("Введите имя файла для child2: ");
    if (fgets(filename2, MAX_STR_LEN, stdin) == NULL) {
        perror("fgets failed");
        exit(EXIT_FAILURE);
    }
    filename2[strcspn(filename2, "\n")] = '\0';

    fd1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    check_file_error(fd1, filename1);
    fd2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    check_file_error(fd2, filename2);

    pid1 = fork();
    check_error(pid1, "fork for child1 failed");
    if (pid1 == 0) {
        close(pipe1[1]);  // Закрываем запись в pipe1
        check_error(dup2(pipe1[0], STDIN_FILENO), "dup2 failed for pipe1 stdin");
        check_error(dup2(fd1, STDOUT_FILENO), "dup2 failed for file1 stdout");
        execlp("./child1", "./child1", NULL);
        perror("execlp failed for child1");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    check_error(pid2, "fork for child2 failed");
    if (pid2 == 0) {
        close(pipe2[1]);
        check_error(dup2(pipe2[0], STDIN_FILENO), "dup2 failed for pipe2 stdin");
        check_error(dup2(fd2, STDOUT_FILENO), "dup2 failed for file2 stdout");
        execlp("./child2", "./child2", NULL);
        perror("execlp failed for child2");
        exit(EXIT_FAILURE);
    }

    close(pipe1[0]);  // Закрываем неиспользуемые концы пайпов
    close(pipe2[0]);

    srand(time(NULL));

    while (1) {
        printf("Введите строку (или 'exit' для завершения): ");
        if (fgets(buffer, MAX_STR_LEN, stdin) == NULL) {
            perror("fgets failed");
            break;
        }
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        if (rand() % 100 < 80) {
            num_bytes = write(pipe1[1], buffer, strlen(buffer) + 1);
            check_error(num_bytes, "write to pipe1 failed");
        } else {
            num_bytes = write(pipe2[1], buffer, strlen(buffer) + 1);
            check_error(num_bytes, "write to pipe2 failed");
        }
    }

    // Закрываем каналы записи в родительском процессе
    close(pipe1[1]);
    close(pipe2[1]);

    // Отправляем сигнал завершения дочерним процессам
    kill(pid1, SIGTERM);
    kill(pid2, SIGTERM);

    // Ожидаем завершения дочерних процессов
    check_error(wait(NULL), "wait failed for child1");
    check_error(wait(NULL), "wait failed for child2");

    close(fd1);
    close(fd2);

    return 0;
}