#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h> 

#define MAX_STR_LEN 1024
#define MAP_SIZE MAX_STR_LEN

typedef struct {
    char data[MAX_STR_LEN];
    int ready;
} shared_memory;

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
    pid_t pid1, pid2;
    char filename1[MAX_STR_LEN], filename2[MAX_STR_LEN];
    char buffer[MAX_STR_LEN];
    int fd_out1, fd_out2;
    int shm_fd1, shm_fd2; 
    shared_memory *map1, *map2;
    ssize_t num_bytes;
    const char *shm_name1 = "/my_shm1"; // Имя для сегмента разделяемой памяти 1
    const char *shm_name2 = "/my_shm2"; // Имя для сегмента разделяемой памяти 2

    printf("Введите имя файла для child1: ");
    if (fgets(filename1, MAX_STR_LEN, stdin) == NULL) {
        perror("fgets failed");
        exit(EXIT_FAILURE);
    }
    filename1[strcspn(filename1, "\n")] = '\0';

    printf("Введите имя файла для child2: ");
    if (fgets(filename2, MAX_STR_LEN, stdin) == NULL) {
        perror("fgets failed");
        exit(EXIT_FAILURE);
    }
    filename2[strcspn(filename2, "\n")] = '\0';

    fd_out1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    check_file_error(fd_out1, filename1);
    fd_out2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    check_file_error(fd_out2, filename2);

    // Создаем и мапим разделяемую память через shm_open
     shm_fd1 = shm_open(shm_name1, O_CREAT | O_RDWR, 0666);
    check_error(shm_fd1, "shm_open failed for shm1");
    ftruncate(shm_fd1, sizeof(shared_memory));
    map1 = (shared_memory *)mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd1, 0);
     check_error((long)map1, "mmap failed for map1");

    shm_fd2 = shm_open(shm_name2, O_CREAT | O_RDWR, 0666);
    check_error(shm_fd2, "shm_open failed for shm2");
    ftruncate(shm_fd2, sizeof(shared_memory));
    map2 = (shared_memory *)mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);
    check_error((long)map2, "mmap failed for map2");

    pid1 = fork();
    check_error(pid1, "fork for child1 failed");
    if (pid1 == 0) {
        close(shm_fd1);
        close(fd_out1);
         execlp("./child1", "./child1", shm_name1, NULL); // Передаем имя разделяемой памяти
        perror("execlp failed for child1");
        exit(EXIT_FAILURE);
    }

    pid2 = fork();
    check_error(pid2, "fork for child2 failed");
    if (pid2 == 0) {
        close(shm_fd2);
        close(fd_out2);
         execlp("./child2", "./child2", shm_name2, NULL); // Передаем имя разделяемой памяти
        perror("execlp failed for child2");
        exit(EXIT_FAILURE);
    }

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
            strcpy(map1->data, buffer);
            map1->ready = 1;

             while (map1->ready) {
                 usleep(100);
             }
            num_bytes = write(fd_out1, map1->data, strlen(map1->data));
            check_error(num_bytes, "Write to output file failed for child1");
             write(fd_out1, "\n", 1);

        } else {
            strcpy(map2->data, buffer);
             map2->ready = 1;
           
            while(map2->ready){
                usleep(100);
            }
            num_bytes = write(fd_out2, map2->data, strlen(map2->data));
            check_error(num_bytes, "Write to output file failed for child2");
             write(fd_out2, "\n", 1);

        }
    }

    // Закрываем и размапируем память
    munmap(map1, sizeof(shared_memory));
    munmap(map2, sizeof(shared_memory));
     close(shm_fd1);
    close(shm_fd2);
    shm_unlink(shm_name1); // Удаляем сегмент разделяемой памяти
    shm_unlink(shm_name2); // Удаляем сегмент разделяемой памяти


    // Отправляем сигнал завершения дочерним процессам
    kill(pid1, SIGTERM);
    kill(pid2, SIGTERM);

    // Ожидаем завершения дочерних процессов
    check_error(wait(NULL), "wait failed for child1");
    check_error(wait(NULL), "wait failed for child2");

    close(fd_out1);
    close(fd_out2);

    return 0;
}