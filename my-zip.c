#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

FILE* open_file(char*, char*);
void write_four_byte_unsigned_int(FILE*, uint32_t);
void write_one_byte_unsigned_int(FILE*, uint8_t);
bool supported_by_ascii(int);
void check_src_dest(FILE*, FILE*);
void zip(FILE*, FILE*);

/* Wrapper with error handling for fopen */
FILE* open_file(char* filename, char* mode) {
    FILE* fp = fopen(filename, mode);
    if (!fp) {
        fprintf(stderr, "my-zip: cannot open file\n");
        exit(1);
    }
    return fp;
}

/* Writes the four_byte_uint into dest as a four byte block in binary format.
    FILE* must be in wb mode (e.g., get FILE* using fopen() beforehand) */
void write_four_byte_unsigned_int(FILE* dest, uint32_t four_byte_uint) {
    if (!dest) {
        fprintf(stderr, "Destination was null.\n");
        exit(1);
    }
    size_t elems_written = fwrite(&four_byte_uint, 4, 1, dest);
    if (elems_written != 1) {
        /* Perror used because global errno was set by fwrite */
        perror("Error in writing to file in write_four_byte_unsigned_int.");
        exit(1);
    }
}

/* Writes the one_byte_uint into dest as a one byte block in binary format.
    FILE* must be in wb mode (e.g., get FILE* using fopen() beforehand) */
void write_one_byte_unsigned_int(FILE* dest, uint8_t one_byte_uint) {
    if (!dest) {
        fprintf(stderr, "Destination was null.\n");
        exit(1);
    }
    size_t elems_written = fwrite(&one_byte_uint, 1, 1, dest);
    if (elems_written != 1) {
        /* Perror used because global errno was set by fwrite */
        perror("Error in writing to file in write_one_byte_unsigned_int.");
        exit(1);
    }
}

bool supported_by_ascii(int c) {
    return (c >= 0 && c <= 127);
}

void insert_if_supported(FILE* dest, int read_character, uint32_t repeat_count) {
    if (supported_by_ascii(read_character)) {
        /* Write the repetition count in a four byte block and the 
        ASCII-char as one byte block*/
        write_four_byte_unsigned_int(dest, repeat_count);
        write_one_byte_unsigned_int(dest, read_character);
    } else {
        fprintf(stderr, "Encountered a non-ASCII supported character: %c, omitting...\n", read_character);
    }
}

void check_src_dest(FILE* src, FILE* dest) {
    if (!src && dest) {
        fclose(dest);
        fprintf(stderr, "Src arg was NULL.\n");
    } else if (src && !dest) {
        fclose(src);
        fprintf(stderr, "Dest arg was NULL.\n");
    } else if (!src && !dest) {
        fprintf(stderr, "Src and dest args were NULL.\n");
    } else {
        return;
    }
    exit(1);
}

void zip(FILE* src, FILE* dest) {
    check_src_dest(src, dest);
    int read_character, prev;
    uint32_t repeatc = 1;
    if ((prev = fgetc(src)) != EOF) {
        while ((read_character = fgetc(src)) != EOF) {
            /* Platform independent maximum value of 32-bit unsigned int, required in case overflow */
            if (read_character == prev && repeatc < 4294967295) {
                repeatc++;
                continue;
            }
            insert_if_supported(dest, prev, repeatc);
            repeatc = 1;
            prev = read_character;
        }
        insert_if_supported(dest, prev, repeatc);
    }
    if (ferror(src) != 0) {
        perror("I/O Error in zip.");
        exit(1);
    } 
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        puts("Usage: ./my-zip file1 [file2 ...]");
        exit(1);
    } else {
        FILE* fp;
        for (argv++; *argv != NULL; argv++) {
            fp = open_file(*argv, "r");
            zip(fp, stdout);
            fclose(fp);
        }
    }
    return 0;
}
