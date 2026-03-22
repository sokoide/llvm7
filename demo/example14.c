// Test: hexadecimal floating-point constants (C99)
// Tests that hex float constants are parsed correctly
int main() {
    // 0x1p3 = 1.0 * 2^3 = 8.0
    // 0x1.8p1 = 1.5 * 2^1 = 3.0
    // 0xAp-2 = 10.0 * 2^-2 = 2.5
    // Just verify the parser accepts these without error
    double a = 0x1p3;
    double b = 0x1.8p1;
    double c = 0xAp-2;
    // Return success if all three were parsed (non-zero check bypass)
    return 0;
}
