#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

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
        
    }   
    free(line);
    return EXIT_SUCCESS;
}