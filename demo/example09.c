int main() {
    struct {
        int a;
        int b;
        int *c;
    } s;

    s.a = 1;
    s.b = 2;
    s.c = &s.a;

    return s.a + s.b + *s.c;
}
