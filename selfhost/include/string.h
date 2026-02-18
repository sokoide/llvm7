/* Minimal string.h for selfhost */
int strncmp(char* s1, char* s2, size_t n);
int strcmp(char* s1, char* s2);
int memcmp(void* s1, void* s2, size_t n);
void* memcpy(void* dest, void* src, size_t n);
void* memset(void* s, int c, size_t n);
char* strncpy(char* dest, char* src, size_t n);
size_t strlen(char* s);
char* strdup(char* s);
char* strstr(char* haystack, char* needle);
char* strchr(char* s, int c);
