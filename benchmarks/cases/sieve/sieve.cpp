#include <cstdio>

int main() {
    long n = 1000000;
    long* s = new long[n];

    long i = 0;
    for (i = 0; i < n; i++) { s[i] = 1; }
    s[0] = 0;
    s[1] = 0;

    for (i = 2; i < n; i++) {
        if (s[i] == 1) {
            long j = i * i;
            while (j < n) {
                s[j] = 0;
                j = j + i;
            }
        }
    }

    long count = 0;
    for (i = 2; i < n; i++) { count = count + s[i]; }
    printf("%ld\n", count);

    delete[] s;
    return 0;
}
