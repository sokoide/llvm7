#include "file_test.h"
#include "../src/file.h"
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* test_read_file_success() {
    char path[] = "test_file_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) {
        perror("mkstemp error");
        return "mkstemp failed";
    }

    FILE* fp = fdopen(fd, "w");
    if (!fp) {
        close(fd);
        unlink(path);
        return "fdopen failed";
    }
    fputs("abc\nxyz\n", fp);
    fclose(fp);

    char* src = read_file(path);
    unlink(path);

    mu_assert("read_file should return non-NULL", src != NULL);
    mu_assert("read content should match", strcmp(src, "abc\nxyz\n") == 0);

    free(src);
    return NULL;
}

char* test_read_file_not_found() {
    char* src = read_file("/tmp/llvm7_this_file_should_not_exist_12345.txt");
    mu_assert("read_file should return NULL for missing file", src == NULL);
    return NULL;
}
