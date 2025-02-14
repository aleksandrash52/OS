#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_THREADS 64
#define BUFFER_SIZE 512


uint64_t global_sum = 0;    
size_t global_count = 0;    
CRITICAL_SECTION cs;        


typedef struct {
    char **lines;
    size_t start_index;
    size_t end_index;
} ThreadData;


DWORD WINAPI process_numbers(LPVOID param) {
    ThreadData *data = (ThreadData *)param;
    uint64_t local_sum = 0;
    size_t local_count = 0;

    for (size_t i = data->start_index; i < data->end_index; ++i) {
        char *line = data->lines[i];
        if (line) {
            uint64_t number = _strtoui64(line, NULL, 16);
            local_sum += number;
            ++local_count;
        }
    }


    EnterCriticalSection(&cs);
    global_sum += local_sum;
    global_count += local_count;
    LeaveCriticalSection(&cs);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <input_file> <max_threads> <max_memory>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    int max_threads = atoi(argv[2]);
    size_t max_memory = atoi(argv[3]);

    if (max_threads <= 0 || max_threads > MAX_THREADS) {
        fprintf(stderr, "Error: max_threads must be between 1 and %d\n", MAX_THREADS);
        return 1;
    }


    FILE *file = fopen(input_file, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }


    size_t num_lines = max_memory / BUFFER_SIZE;
    char **lines = malloc(num_lines * sizeof(char *));
    if (!lines) {
        perror("Memory allocation error");
        fclose(file);
        return 1;
    }


    size_t line_count = 0;
    while (line_count < num_lines && !feof(file)) {
        lines[line_count] = malloc(BUFFER_SIZE);
        if (!lines[line_count]) {
            perror("Memory allocation error");
            break;
        }
        if (fgets(lines[line_count], BUFFER_SIZE, file)) {
            lines[line_count][strcspn(lines[line_count], "\n")] = '\0';
            ++line_count;
        }
    }
    fclose(file);

    if (line_count == 0) {
        fprintf(stderr, "Error: No valid lines to process.\n");
        free(lines);
        return 1;
    }


    InitializeCriticalSection(&cs);


    HANDLE threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    size_t lines_per_thread = line_count / max_threads;

    for (int i = 0; i < max_threads; ++i) {
        size_t start_index = i * lines_per_thread;
        size_t end_index = (i == max_threads - 1) ? line_count : start_index + lines_per_thread;

        thread_data[i].lines = lines;
        thread_data[i].start_index = start_index;
        thread_data[i].end_index = end_index;

        threads[i] = CreateThread(NULL, 0, process_numbers, &thread_data[i], 0, NULL);
        if (!threads[i]) {
            fprintf(stderr, "Error creating thread %d\n", i);
            DeleteCriticalSection(&cs);
            for (int j = 0; j < i; ++j) {
                CloseHandle(threads[j]);
            }
            for (size_t k = 0; k < line_count; ++k) {
                free(lines[k]);
            }
            free(lines);
            return 1;
        }
    }


    WaitForMultipleObjects(max_threads, threads, TRUE, INFINITE);

    for (int i = 0; i < max_threads; ++i) {
        CloseHandle(threads[i]);
    }


    DeleteCriticalSection(&cs);


    if (global_count > 0) {
        uint64_t average = global_sum / global_count;
        printf("average: %llu\n", average);
    } else {
        printf("none\n");
    }


    for (size_t i = 0; i < line_count; ++i) {
        free(lines[i]);
    }
    free(lines);

    return 0;
}