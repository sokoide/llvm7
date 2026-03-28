/* Minimal stdlib.h for selfhost */
void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void free(void* ptr);
void exit(int status);
long strtol(char* str, char** endptr, int base);
double strtod(const char *nptr, char **endptr);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
char *getenv(const char *name);
void *realloc(void *ptr, size_t size);
