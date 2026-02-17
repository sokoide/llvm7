int main() {
    int x;
    int* y;
    int ar[10];

    x = 3;
    y = &x;
    *y = 5;

    for (x = 0; x < 3; x = x + 1) {
        ar[x] = x + 10;
    }
    return ar[2];
}
