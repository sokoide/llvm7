int main() {
    int x = 0;
    goto L;
    x = 1;
L:
    return x + 42;
}
