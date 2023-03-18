#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5000

void concatenate(char *s1, char *s2, char *output) {
    int ascii[128] = {0}; // initialize all ASCII values to 0
    int count = 0;

    for (int i = 0; i < strlen(s1); i++) {
        if (s1[i] == '\n') {
            continue;
        }
        if (ascii[(int) s1[i]] == 0) { // if the character has not been encountered before
            ascii[(int) s1[i]] = 1; // mark it as encountered
            output[count] = s1[i]; // add it to the output string
            count++;
        }
    }

    for (int i = 0; i < strlen(s2); i++) {
        if (ascii[(int) s2[i]] == 0) { // if the character has not been encountered before
            ascii[(int) s2[i]] = 1; // mark it as encountered
            output[count] = s2[i]; // add it to the output string
            count++;
        }
    }

    output[count] = '\0'; // add the null terminator at the end of the output string
}

int main(int argc, char *argv[]) {
    int fifo1, fifo2;
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

    // Создаем именованные каналы
    if (mkfifo("fifo1", 0666) == -1 || mkfifo("fifo2", 0666) == -1) {
        perror("Error creating named pipes");
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

        // Читаем данные из файла
        char input_buffer[BUFFER_SIZE];
        size_t input_size = fread(input_buffer, 1, BUFFER_SIZE, input_file);
        fclose(input_file);

        // Write the input buffer to the first named pipe
        fifo1 = open("fifo1", O_WRONLY);
        write(fifo1, input_buffer, input_size);
        close(fifo1);

        // Read the result from the second named pipe
        fifo2 = open("fifo2", O_RDONLY);
        read(fifo2, result, BUFFER_SIZE);
        close(fifo2);

        // Write the result to the output file
        fwrite(result, sizeof(char), strlen(result), output_file);

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

        // Read the input buffer from the first named pipe
        fifo1 = open("fifo1", O_RDONLY);
        read(fifo1, buffer1, BUFFER_SIZE);
        close(fifo1);

        // Запускаем обработку данных
        concatenate(buffer1, "", result);

        // Write the result to the second named pipe
        fifo2 = open("fifo2", O_WRONLY);
        write(fifo2, result, strlen(result) + 1);
        close(fifo2);

        // Завершаем работу процесса
        exit(0);
    }

    // Закрываем неиспользуемые концы каналов
    close(fifo1);
    close(fifo2);

    // Ожидаем завершения всех дочерних процессов
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    // Удаляем именованные каналы
    unlink("fifo1");
    unlink("fifo2");

    return 0;
}
