#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMANDS 200
#define MAX_ARGS 256 

void parse_args(char* command, char **args, int max_args);

int main() {
    char command[256];
    char *commands[MAX_COMMANDS];
    int command_count = 0;
    char *args[MAX_ARGS + 1];  // +1 for NULL at the end

    while (1) {
        printf("Shell> ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';

        char *token = strtok(command, "|");
        while (token != NULL) {
            commands[command_count++] = token;
            token = strtok(NULL, "|");
        }

        int pipes[command_count-1][2];
        for (int i = 0; i < command_count-1; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("Error al crear pipe");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < command_count; i++) {
            parse_args(commands[i], args, MAX_ARGS);
            pid_t pid = fork();

            if (pid == -1) {
                perror("Fork fallido");
                exit(EXIT_FAILURE);

            } else if (pid == 0) { // Proceso hijo
                
                if (i == 0) { // Primer hijo
                    for (int j = 1; j < command_count - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    close(pipes[0][0]);

                } else if (i == command_count - 1) { // Último hijo
                    for (int j = 0; j < command_count - 2; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    close(pipes[command_count - 2][1]);

                } else { // Hijos intermedios (0 < i < command_count - 1)
                    for (int j = 0; j < command_count-1; j++) {
                        if (j != i - 1) {
                            close(pipes[j][0]);
                        }
                        if (j != i) {
                            close(pipes[j][1]);
                        }
                    }
                }

                if (i > 0) {
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                    close(pipes[i - 1][0]);
                }

                if (i < command_count - 1) {
                    dup2(pipes[i][1], STDOUT_FILENO);
                    close(pipes[i][1]);
                }

                execvp(args[0], args);  
                exit(1);
            }
        }

        for (int i = 0; i < command_count - 1; i++) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }

        for (int i = 0; i < command_count; i++) {
            if (wait(NULL) == -1) {
                perror("Error en el wait");
                exit(EXIT_FAILURE);
            }
        }

        command_count = 0;
    }

    return 0;
}

void parse_args(char* command, char **args, int max_args) {
    int i = 0;
    char *p = command;
    char *start_of_word;

    for (int j = 0; j <= max_args; j++) {
        args[j] = NULL;
    }

    while (*p) {
        while (isspace((unsigned char)*p)) p++;

        if (*p == '\0') {
            break;
        }

        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            start_of_word = p;
            while (*p && *p != quote) p++;
        } else {
            start_of_word = p;
            while (*p && !isspace((unsigned char)*p) && *p != '"' && *p != '\'') p++;
        }

        if (i < max_args) {
            args[i++] = start_of_word;
        }

        if (*p) {
            *p++ = '\0';
        }
    }
}
