#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5000

void concatenate(char* s1, char* s2, char* output) {
    int ascii[128] = {0}; // initialize all ASCII values to 0
    int len = strlen(s1) + strlen(s2);
    int count = 0;
    char final_output[BUFFER_SIZE];

    for (int i = 0; i < strlen(s1); i++) {
        if (s1[i] == '\n') {
            continue;
        }
        if (ascii[(int)s1[i]] == 0) { // if the character has not been encountered before
            ascii[(int)s1[i]] = 1; // mark it as encountered
            output[count] = s1[i]; // add it to the output string
            count++;
        }
    }

    for (int i = 0; i < strlen(s2); i++) {
        if (ascii[(int)s2[i]] == 0) { // if the character has not been encountered before
            ascii[(int)s2[i]] = 1; // mark it as encountered
            output[count] = s2[i]; // add it to the output string
            count++;
        }
    }

    output[count] = '\0'; // add the null terminator at the end of the output string
}

int main(int argc, char *argv[]) {
    int pipefd[2], pipefd2[2];
    pid_t pid1, pid2;

    char buffer1[BUFFER_SIZE], buffer2[BUFFER_SIZE], result[BUFFER_SIZE];

    // Проверяем, есть ли файлы для чтения и записи
    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    // Открываем файлы для чтения и записи
    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        perror("Error opening input file");
        return 1;
    }

    FILE *output_file = fopen(argv[2], "w");
    if (output_file == NULL) {
        perror("Error opening output file");
        return 1;
    }

    // Создаем неименованные каналы
    if (pipe(pipefd) == -1 || pipe(pipefd2) == -1) {
        perror("Error creating pipes");
        return 1;
    }

    // Создаем первый процесс
    pid1 = fork();

    if (pid1 == -1) {
        perror("Error forking first process");
        return 1;
    }

    if (pid1 == 0) {
    // Дочерний процесс - первый процесс

    // Закрываем неиспользуемые концы каналов
    close(pipefd[0]);
    close(pipefd2[1]);

    // Read the input file into a buffer
    char input_buffer[BUFFER_SIZE];
    size_t input_size = fread(input_buffer, 1, BUFFER_SIZE, input_file);
    fclose(input_file);

    // Write the input buffer to the first pipe
    write(pipefd[1], input_buffer, input_size);

    // Read the result from the second pipe
    read(pipefd2[0], result, BUFFER_SIZE);

    // Write the result to the output file
    fprintf(output_file, "%s", result);

    // Завершаем работу процесса
    exit(0);
}

// Создаем второй процесс
pid2 = fork();

if (pid2 == -1) {
    perror("Error forking second process");
    return 1;
}

if (pid2 == 0) {
    // Дочерний процесс - второй процесс

    // Закрываем неиспользуемые концы каналов
    close(pipefd[1]);
    close(pipefd2[0]);

    // Read the input buffer from the first pipe
    read(pipefd[0], buffer1, BUFFER_SIZE);

    // Запускаем обработку данных
    concatenate(buffer1, "", result);

    // Write the result to the second pipe
    write(pipefd2[1], result, strlen(result)+1);

    // Завершаем работу процесса
    exit(0);
}

// Родительский процесс

// Закрываем неиспользуемые концы каналов
close(pipefd[0]);
close(pipefd[1]);
close(pipefd2[0]);
close(pipefd2[1]);

// Ожидаем завершения всех дочерних процессов
waitpid(pid1, NULL, 0);
waitpid(pid2, NULL, 0);

return 0;
}
