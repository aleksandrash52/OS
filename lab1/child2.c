#include <stdio.h>
#include <string.h>
#include <unistd.h>

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

int main() {
    char buffer[1024];
    ssize_t num_bytes;

    // Чтение строк из stdin
    while ((num_bytes = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        buffer[strcspn(buffer, "\n")] = '\0';
        reverse_string(buffer);
        if (printf("%s\n", buffer) < 0) {
            perror("printf failed");
            return 1;
        }
        fflush(stdout);
    }

    if (num_bytes == -1) {
        perror("read failed");
        return 1;
    }

    return 0;
}