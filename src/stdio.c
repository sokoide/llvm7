// Minimal I/O functions for self-hosting
// DISABLED: We link against libc, so we don't need this dummy implementation.
// Keeping file to satisfy Makefile wildcard, but empty.

#if 0
#include <stdlib.h>

// ... (original content commented out) ...
#endif

void init_stdio(void) {
    // No-op
}
