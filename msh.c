#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t characters_read;

    while (1)
    {
        printf("msh> ");
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
        char ** args = malloc(((size_t)characters_read + 1)*sizeof(*args));

        if(args == NULL){
            perror("malloc");
            free(line);
            return EXIT_FAILURE;
        }

        size_t count = 0;
        char*token = strtok(line, " \t\n");

        while(token != NULL){
            args[count] = token;
            count++;
            token = strtok(NULL, " \t\n");
        }
        args[count] = NULL;

        if(count > 0){
            for (size_t i = 0; args[i] != NULL; i++){
                printf("argv[%zu] = %s\n", i, args[i]);
            }
        }
        
        free(args);
    }   
    free(line);
    return EXIT_SUCCESS;
}