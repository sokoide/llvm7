int main() {
    int x;
    int y;

    x = 3;
    y = &x;
    *y = 5;
    return x;
}
