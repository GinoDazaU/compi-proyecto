#include <cstdio>

int main() {
    long n = 1000000;
    long total = 0;

    for (long i = 1; i <= n; i++) {
        long x = i;
        while (x != 1) {
            if (x % 2 == 0) {
                x = x / 2;
            } else {
                x = 3 * x + 1;
            }
            total = total + 1;
        }
    }

    printf("%ld\n", total);
    return 0;
}
