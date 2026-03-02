#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_file(const char* filename);

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return NULL;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char* source = malloc((size_t)file_size + 1);
    if (source == NULL) {
        fclose(file);
        return NULL;
    }

    size_t nread = fread(source, 1, (size_t)file_size, file);
    if (nread != (size_t)file_size && ferror(file)) {
        free(source);
        fclose(file);
        return NULL;
    }
    fclose(file);
    source[nread] = '\0';

    return source;
}
