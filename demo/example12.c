int printf(const char* fmt, ...);

int main() {
    float f = 1.23f;
    double d = 2.34;

    printf("float f = %f\n", (double)f);
    printf("double d = %f\n", d);

    float sum = f + (float)d;
    printf("sum = %f\n", (double)sum);

    int i = 0;
    do {
        i = i + 1;
    } while (i < 5);
    printf("do-while i = %d\n", i);

    float res = (i > 3) ? f : (float)d;
    printf("ternary res = %f\n", (double)res);

    return 0;
}
