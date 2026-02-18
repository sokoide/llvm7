/* Minimal stdio.h for selfhost */
typedef struct {
    int fd;
} FILE;
extern FILE* stderr;
extern FILE* stdout;
#define SEEK_SET 0
#define SEEK_END 2

// Initialize stdio streams
void init_stdio(void);

int printf(const char* fmt, ...);
int fprintf(FILE* stream, const char* fmt, ...);
int snprintf(char* buf, int size, const char* fmt, ...);
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* file);
int fseek(FILE* file, long offset, int whence);
long ftell(FILE* file);
size_t fread(void* ptr, size_t size, size_t count, FILE* stream);
int fflush(FILE* stream);
