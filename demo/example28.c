int main() {
    int x = 2;
    int z = 0;
    switch (x) {
    case 1:
    case 2:
        z = 100;
    case 3:
        z = z + 1;
        break;
    }
    return z;
}
