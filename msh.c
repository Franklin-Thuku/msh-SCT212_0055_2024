
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t characters_read;
    int last_status = 0;

    while (1)
    {
        printf("msh[%d]> ", last_status);
        fflush(stdout);

        characters_read = getline(&line, &capacity, stdin);

        if (characters_read == -1)
        {
            if (feof(stdin))
            {
                printf("\n");
                break;
            }

            perror("getline");
            free(line);
            return EXIT_FAILURE;
        }

        char **args = malloc(((size_t)characters_read + 1) * sizeof(*args));

        if (args == NULL)
        {
            perror("malloc");
            free(line);
            return EXIT_FAILURE;
        }

        size_t count = 0;
        char *token = strtok(line, " \t\n");

        while (token != NULL)
        {
            args[count++] = token;
            token = strtok(NULL, " \t\n");
        }

        args[count] = NULL;

        int exit_requested = 0;

        if (count > 0)
        {
            if (strcmp(args[0], "cd") == 0)
            {
                if (count > 2)
                {
                    fprintf(stderr, "cd: too many arguments\n");
                    last_status = 1;
                }
                else
                {
                    const char *directory = count == 1 ? getenv("HOME") : args[1];

                    if (directory == NULL)
                    {
                        fprintf(stderr, "cd: HOME not set\n");
                        last_status = 1;
                    }
                    else if (chdir(directory) == -1)
                    {
                        perror("cd");
                        last_status = 1;
                    }
                    else
                    {
                        last_status = 0;
                    }
                }
            }
            else if (strcmp(args[0], "help") == 0)
            {
                printf("msh - Minimal Shell\n");
                printf("Available commands:\n");
                printf("  cd [directory]  Change the current directory\n");
                printf("  help            Display this help message\n");
                printf("  exit            Exit the shell\n");
                printf("  Other commands  Execute external programs\n");
                last_status = 0;
            }
            else if (strcmp(args[0], "exit") == 0)
            {
                if (count > 1)
                {
                    fprintf(stderr, "exit: too many arguments\n");
                    last_status = 1;
                }
                else
                {
                    exit_requested = 1;
                }
            }
            else
            {
                pid_t pid = fork();

                if (pid < 0)
                {
                    perror("fork");
                    last_status = 1;
                }
                else if (pid == 0)
                {
                    execvp(args[0], args);
                    perror(args[0]);
                    _exit(127);
                }
                else
                {
                    int status;
                    pid_t result;

                    do
                    {
                        result = waitpid(pid, &status, 0);
                    }
                    while (result == -1 && errno == EINTR);

                    if (result == -1)
                    {
                        perror("waitpid");
                        last_status = 1;
                    }
                    else if (WIFEXITED(status))
                    {
                        last_status = WEXITSTATUS(status);
                    }
                    else if (WIFSIGNALED(status))
                    {
                        last_status = 128 + WTERMSIG(status);
                    }
                }
            }
        }

        free(args);

        if (exit_requested)
        {
            break;
        }
    }

    free(line);

    return EXIT_SUCCESS;
}
