// Minimal I/O functions for self-hosting

#include <stdlib.h>

// Declare write function (will be resolved by libc)
extern long write(int fd, const void* buf, unsigned long count);

// Simple FILE structure for self-hosting
typedef struct {
    int fd;
} FILE;

// Standard file descriptors (exported)
FILE* stdin;
FILE* stdout;
FILE* stderr;

// Helper to write a character
static void write_char(int fd, char c) {
    char buf[1] = {c};
    write(fd, buf, 1);
}

// Helper to write a string
static void write_str(int fd, const char* s) {
    if (!s)
        s = "(null)";
    long len = 0;
    const char* p = s;
    while (*p) {
        p++;
        len++;
    }
    write(fd, s, len);
}

// Helper to write an integer
static void write_int(int fd, int n) {
    if (n < 0) {
        write_char(fd, '-');
        n = -n;
    }
    if (n >= 10) {
        write_int(fd, n / 10);
    }
    write_char(fd, '0' + (n % 10));
}

// fprintf - simplified version without format processing
int fprintf(FILE* stream, const char* fmt, ...) {
    int fd = (stream == stderr) ? 2 : 1;
    // For now, just write the format string as-is
    // This means format specifiers like %s will be printed literally
    write_str(fd, fmt);
    return 0;
}

// Minimal fwrite implementation
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
    int fd = (stream == stderr) ? 2 : 1;
    return write(fd, ptr, size * count);
}

// Minimal printf implementation
int printf(const char* fmt, ...) { return fprintf(stdout, fmt); }

// Helper to write error message
int perror(const char* s) {
    write_str(2, s);
    write_str(2, "\n");
    return 0;
}

// Initialize stdio streams (called from main)
void init_stdio(void) {
    // Allocate FILE structures
    stdin = malloc(sizeof(FILE));
    stdout = malloc(sizeof(FILE));
    stderr = malloc(sizeof(FILE));

    // Set file descriptors
    if (stdin)
        stdin->fd = 0;
    if (stdout)
        stdout->fd = 1;
    if (stderr)
        stderr->fd = 2;
}
