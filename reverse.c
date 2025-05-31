#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INITIAL_CAPACITY 8

int main(int argc, char *argv[]) {
    // handle the number of command-line arguments
    if (argc > 3) {
        fprintf(stderr, "usage: reverse <input> <output>\n");
        exit(1);
    }

    FILE *inputFile = stdin;    // default input is stdin
    FILE *outputFile = stdout;  // default output is stdout

    // open input file if provided
    if (argc >= 2) {
        inputFile = fopen(argv[1], "r");
        if (inputFile == NULL) {
            fprintf(stderr, "error: cannot open file '%s'\n", argv[1]);
            exit(1);
        }
    }

    // open output file if provided
    if (argc == 3) {
        // make sure input and output files are different
        if (strcmp(argv[1], argv[2]) == 0) {
            fprintf(stderr, "Input and output file must differ\n");
            if (inputFile != stdin)
                fclose(inputFile);
            exit(1);
        }

        outputFile = fopen(argv[2], "w");
        if (outputFile == NULL) {
            fprintf(stderr, "error: cannot open file '%s'\n", argv[2]);
            if (inputFile != stdin)
                fclose(inputFile);
            exit(1);
        }
    }

    // allocate memory for storing lines
    size_t capacity = INITIAL_CAPACITY; // initial array size
    size_t count = 0;                   // number of lines read
    char **lines = malloc(sizeof(char *) * capacity);
    if (lines == NULL) {
        fprintf(stderr, "malloc failed\n");
        if (inputFile != stdin)
            fclose(inputFile);
        if (outputFile != stdout)
            fclose(outputFile);
        exit(1);
    }

    char *buffer = NULL;    // buffer used by getline()
    size_t bufsize = 0;     // buffer size for getline()
    ssize_t linelen;

    // read lines until end of file
    while ((linelen = getline(&buffer, &bufsize, inputFile)) != -1) {
        // expand the array if it's full
        if (count == capacity) {
            capacity *= 2;
            char **temp = realloc(lines, sizeof(char *) * capacity);
            if (temp == NULL) {
                free(buffer);
                fprintf(stderr, "malloc failed\n");
                if (inputFile != stdin)
                    fclose(inputFile);
                if (outputFile != stdout)
                    fclose(outputFile);

                // free previously allocated lines
                for (size_t i = 0; i < count; i++) {
                    free(lines[i]);
                }
                free(lines);
                exit(1);
            }
            lines = temp;
        }

        // allocate memory for the line and copy it
        lines[count] = malloc((linelen + 1) * sizeof(char));
        if (lines[count] == NULL) {
            free(buffer);
            fprintf(stderr, "malloc failed\n");
            if (inputFile != stdin)
                fclose(inputFile);
            if (outputFile != stdout)
                fclose(outputFile);

            // free already allocated lines
            for (size_t i = 0; i < count; i++) {
                free(lines[i]);
            }
            free(lines);
            exit(1);
        }

        strncpy(lines[count], buffer, linelen + 1);
        count++;
    }

    free(buffer); // free the buffer used by getline()

    // print lines in reverse order
    for (ssize_t i = count - 1; i >= 0; i--) {
        fprintf(outputFile, "%s", lines[i]);
        free(lines[i]); // free each line after printing
    }

    free(lines); // free the array holding the pointers

    // close files if they were opened
    if (inputFile != stdin)
        fclose(inputFile);
    if (outputFile != stdout)
        fclose(outputFile);

    return 0; // program completed successfully
}
