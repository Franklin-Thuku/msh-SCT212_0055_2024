
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#define MAX_JOBS 1024

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t characters_read;
    int last_status = 0;

    pid_t background_jobs[MAX_JOBS];
    size_t job_count = 0;

    while (1)
    {
        for (size_t i = 0; i < job_count;)
        {
            int status;
            pid_t result = waitpid(background_jobs[i], &status, WNOHANG);

            if (result > 0)
            {
                printf("[Done] PID %d\n", result);

                for (size_t j = i; j + 1 < job_count; j++)
                {
                    background_jobs[j] = background_jobs[j + 1];
                }

                job_count--;
            }
            else if (result == -1 && errno == ECHILD)
            {
                for (size_t j = i; j + 1 < job_count; j++)
                {
                    background_jobs[j] = background_jobs[j + 1];
                }

                job_count--;
            }
            else
            {
                i++;
            }
        }

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

            if (errno == EINTR)
            {
                clearerr(stdin);
                continue;
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
        int background = 0;

        if (count > 0)
        {
            if (strcmp(args[count - 1], "&") == 0)
            {
                background = 1;
                args[--count] = NULL;
            }

            if (count == 0)
            {
                free(args);
                continue;
            }

            if (!background && strcmp(args[0], "cd") == 0)
            {
                if (count > 2)
                {
                    fprintf(stderr, "cd: too many arguments\n");
                    last_status = 1;
                }
                else
                {
                    const char *directory =
                        count == 1 ? getenv("HOME") : args[1];

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
            else if (!background && strcmp(args[0], "help") == 0)
            {
                printf("msh - Minimal Shell\n");
                printf("Available commands:\n");
                printf("  cd [directory]  Change directory\n");
                printf("  help            Display help\n");
                printf("  exit            Exit the shell\n");
                printf("  command &       Run command in background\n");
                printf("  Other commands  Execute external programs\n");
                last_status = 0;
            }
            else if (!background && strcmp(args[0], "exit") == 0)
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
                else if (background)
                {
                    if (job_count < MAX_JOBS)
                    {
                        background_jobs[job_count++] = pid;
                        printf("[Background] PID %d\n", pid);
                    }
                    else
                    {
                        fprintf(stderr, "msh: background job limit reached\n");
                        waitpid(pid, NULL, 0);
                        last_status = 1;
                    }
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

    for (size_t i = 0; i < job_count; i++)
    {
        waitpid(background_jobs[i], NULL, 0);
    }

    return EXIT_SUCCESS;
}
