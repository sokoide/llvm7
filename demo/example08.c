// line comment
int g;
int main() {
    int x; // comment
    int* y;
    int ar[10];

    /*
     * comment
     */
    x = 3;
    y = &x;
    *y = 5;
    g = 1;

    for (x = 0; x < 3; x = x + 1) {
        ar[x] = x + 10 + g;
    }
    return ar[2];
}
