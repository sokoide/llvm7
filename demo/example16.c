void set(int* p, int v) { *p = v; }
int main() {
    int x = 0;
    set(&x, 99);
    return x;
}
