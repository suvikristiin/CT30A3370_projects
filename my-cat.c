#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

FILE* open_file(char*, char*);
bool get_line(char**, FILE*);

/* Wrapper with error handling for fopen */
FILE* open_file(char* filename, char* mode) {
    FILE* fp = fopen(filename, mode);
    if (!fp) {
        /* If the program tries to fopen() a file and fails,
         it should print the exact message "my-cat: cannot open file" */
        fprintf(stderr, "my-cat: cannot open file\n");
        exit(1);
    }
    return fp;
}

/* Reads a whole line from input_stream to line, returns false in case of EOF */
bool get_line(char** line, FILE* input_stream) {
    if (!input_stream) {
        fprintf(stderr, "my-cat: input stream was NULL\n");
        exit(1);
    }
    *line = NULL;
    size_t size_of_line = 0;
    ssize_t characters_read = getline(line, &size_of_line, input_stream);
    if (characters_read == -1) {
        if (ferror(input_stream) != 0) {
            /* Global errno is set if getline fails and returns -1, also returns -1 on EOF.
             According to man pages, *line should be free'd anyway. */
            perror("my-cat");
            free(*line);
            exit(1);
        }
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    /* If no files are specified on the command line, my-cat should just exit and return 0. */
    if (argc > 1) {
        /* Your program my-cat can be invoked with one or more files on the command line;
         it should just print out each file in turn. */
        FILE* fp;
        char* line;
        for (argv++; *argv != NULL; argv++) {
            line = NULL;
            fp = open_file(*argv, "r");
            while (get_line(&line, fp)) {
                printf("%s", line);
                free(line);
            }
            free(line);
            fclose(fp);
        }
    }
    return 0;
}
