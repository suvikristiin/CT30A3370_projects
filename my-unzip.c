#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

FILE* open_file(char* file_name) {
    FILE* f = fopen(file_name, "rb");
	if (!f) {
        fprintf(stderr, "my-unzip: couldn't open file\n");
        exit(1);
    }
    return f;
}

void unzip(char* file_name) {
    FILE* fptr = open_file(file_name);
    uint32_t repeated_count;
    uint8_t ascii;
    size_t read_count;
    int i;
    /* fread guaranteed to return zero in case of error or EOF in this case because nmemb is exactly one */
    /* otherwise always returns 1. In other cases use while (fread != nmemb). */
    while ((read_count = fread(&repeated_count, 4, 1, fptr))) {
        read_count = fread(&ascii, 1, 1, fptr);
        if (!read_count) break;
        for (i = 0; i < repeated_count; i++) {
            fprintf(stdout, "%c", ascii);
        }
    }
    if (ferror(fptr) != 0) {
        perror("my-unzip");
        exit(1);
    }
    fclose(fptr);
}

int main (int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./my-unzip: file1 [file2 ...]\n");
        exit(1);
    }
    /* Loop the file names */
    for (argv++; *argv != NULL; argv++) {
        unzip(*argv);
    }
	return 0;
}
