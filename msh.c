
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t characters_read;

    while (1)
    {
        /* Display prompt */
        printf("msh> ");
        fflush(stdout);

        /* Read user input */
        characters_read = getline(&line, &capacity, stdin);

        /* Handle end of input or read errors */
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

        /* Allocate memory for command arguments */
        char **args = malloc(((size_t)characters_read + 1) * sizeof(*args));

        if (args == NULL)
        {
            perror("malloc");
            free(line);
            return EXIT_FAILURE;
        }

        /* Tokenize input into arguments */
        size_t count = 0;
        char *token = strtok(line, " \t\n");

        while (token != NULL)
        {
            args[count] = token;
            count++;

            token = strtok(NULL, " \t\n");
        }

        /* Terminate argument array with NULL */
        args[count] = NULL;

        /* Execute command only if input is not blank */
        if (count > 0)
        {
            pid_t pid = fork();

            if (pid < 0)
            {
                /* Handle fork failure */
                perror("fork");
            }
            else if (pid == 0)
            {
                /* Child process */
                execvp(args[0], args);

                /* execvp returns only when execution fails */
                perror(args[0]);
                _exit(127);
            }
            else
            {
                /* Parent process */
                int status;

                if (waitpid(pid, &status, 0) == -1)
                {
                    perror("waitpid");
                }
            }
        }

        /* Free argument array */
        free(args);
    }

    /* Free input buffer */
    free(line);

    return EXIT_SUCCESS;
}
