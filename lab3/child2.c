#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/shm.h> // Добавляем заголовочный файл для shm_open

#define MAX_STR_LEN 1024
#define MAP_SIZE MAX_STR_LEN

typedef struct {
    char data[MAX_STR_LEN];
    int ready;
} shared_memory;

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

volatile sig_atomic_t terminate = 0;

void sigterm_handler(int signum) {
    terminate = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <shm_name>\n", argv[0]);
        return 1;
    }
    int shm_fd;
    shared_memory *map;

    shm_fd = shm_open(argv[1], O_RDWR, 0); // Открываем сегмент разделяемой памяти
    if (shm_fd == -1) {
        perror("Failed to open shared memory in child1");
        return 1;
    }
    map = (shared_memory *)mmap(NULL, sizeof(shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap failed in child1");
        close(shm_fd);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    while (!terminate) {
        if (map->ready) {
            reverse_string(map->data);
            map->ready = 0;
        }
         usleep(100);
    }
    munmap(map, sizeof(shared_memory));
     close(shm_fd);

    return 0;
}