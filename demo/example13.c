// Test: __func__ predefined identifier (C99)
int main() {
    // __func__ should be "main"
    const char* name = __func__;
    // Check first character is 'm'
    int result = (name[0] == 'm') ? 0 : 1;
    // Check second character is 'a'
    if (name[1] != 'a') return 1;
    // Check third character is 'i'
    if (name[2] != 'i') return 1;
    // Check fourth character is 'n'
    if (name[3] != 'n') return 1;
    // Check null terminator
    if (name[4] != 0) return 1;
    return 0;
}
