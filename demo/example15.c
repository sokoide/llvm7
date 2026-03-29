int main() {
    int s = 0;
    int i;
    for (i = 1; i <= 10; i = i + 1) {
        if (i % 2 == 0)
            continue;
        s = s + i;
    }
    return s;
}
