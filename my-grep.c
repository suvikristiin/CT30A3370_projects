#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

FILE* open_file(char*);
bool get_line(char**, FILE*);

bool get_line(char** line, FILE* input_stream) {
    if (!input_stream) {
        fprintf(stderr, "my-grep: input stream was NULL\n");
        exit(1);
    }
    *line = NULL;
    size_t size_of_line = 0;
    ssize_t characters_read = getline(line, &size_of_line, input_stream);
    if (characters_read == -1) {
        if (ferror(input_stream) != 0) {
            /* Global errno is set if getline fails and returns -1, also returns -1 on EOF.
             According to man pages, *line should be free'd anyway. */
            perror("my-grep");
            free(*line);
            exit(1);
        }
        return false;
    }
    return true;
}

FILE* open_file(char* file_name) {
    /* If the file_name = NULL, return stdout.
     * Otherwise try to open the specified file */
    FILE* file_to_read;
    if (file_name) {
        if ((file_to_read = fopen(file_name, "r")) == NULL) {
            fprintf(stderr, "my-grep: cannot open file '%s'\n", file_name);
            exit(1);
        }
    }
    else {
        file_to_read = stdin;
    }
    return file_to_read;
}

void search_word(char* word, char* file_name) {
    FILE* file_to_read = open_file(file_name);
    /* i is used to loop through the read line */
    /* j is used to check the same characters */
    int i, j;
    size_t line_len, word_len;
    char* line = NULL;
    while (get_line(&line, file_to_read)) {
        j = 0;
        /* Break user input on an empty line */
        if (!file_name && line[0] == '\n')
            break;
        line_len = strlen(line);
        word_len = strlen(word);
        /* Loop through the line and compare chars */
        for (i = 0; i < line_len; i++) {
            if (line[i] == word[j]) {
                j++;
            }
            else {
                j = 0;
            }
            /* Check if the word has been found */
            if (j == word_len) {
                fprintf(stdout, "%s", line);
                break;
            }
        }
        free(line);
    }
    free(line);
    /* Close the file if not stdin */
    if (file_name)
        fclose(file_to_read);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "my-grep: searchterm [file...]\n");
        exit(1);
    }
    /* Loop the file names */
    if (argc == 2) {
        search_word(argv[1], NULL);
    }
    else {
        int i = 2;
        for (; i < argc; i++) {
            search_word(argv[1], argv[i]);
        }
    }
    return 0;
}
