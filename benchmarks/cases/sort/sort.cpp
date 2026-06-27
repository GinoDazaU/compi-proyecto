#include <cstdio>

int main() {
    static long a[10000];

    for (long i = 0; i < 10000; i++) {
        a[i] = (i * 31 + 7) % 1000;
    }

    for (long i = 0; i < 9999; i++) {
        for (long j = 0; j < 9999 - i; j++) {
            if (a[j] > a[j + 1]) {
                long tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
            }
        }
    }

    long chk = 0;
    for (long i = 0; i < 10000; i++) {
        chk = chk + a[i] * i;
    }
    printf("%ld\n", chk);
    return 0;
}
