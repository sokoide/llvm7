int fib(int a) {
    if (a <= 1) {
        return a;
    }
    return fib(a - 1) + fib(a - 2);
}

int main() { return fib(10); }