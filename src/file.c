#include <stdio.h>
#include <stdlib.h>

char* read_file(const char* filename);

char* read_file(const char* filename) {
    char* source;

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    source = malloc(file_size + 1);
    fread(source, 1, file_size, file);
    fclose(file);
    source[file_size] = '\0';

    return source;
}