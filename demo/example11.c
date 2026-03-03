int printf(const char* fmt, ...);

int main() {
    double x = 1.23;
    double y = 2.34;
    double z = x + y;
    printf("x = %f\n", x);
    printf("y = %f\n", y);
    printf("x + y = %f\n", z);

    if (z > 3.5) {
        printf("z > 3.5\n");
    } else {
        printf("z <= 3.5\n");
    }

    int i = (int)z;
    printf("(int)z = %d\n", i);

    double w = (double)i;
    printf("(double)i = %f\n", w);

    return 0;
}
