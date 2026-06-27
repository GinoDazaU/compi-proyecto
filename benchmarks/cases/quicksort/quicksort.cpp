#include <cstdio>

void quicksort(long* a, long lo, long hi) {
    if (lo >= hi) { return; }
    long pivot = a[hi];
    long i = lo - 1;
    for (long j = lo; j < hi; j++) {
        if (a[j] < pivot) {
            i++;
            long t = a[i];
            a[i] = a[j];
            a[j] = t;
        }
    }
    long p = i + 1;
    long t = a[p];
    a[p] = a[hi];
    a[hi] = t;
    quicksort(a, lo, p - 1);
    quicksort(a, p + 1, hi);
}

int main() {
    long n = 1000000;
    long* a = new long[n];
    for (long i = 0; i < n; i++) {
        a[i] = (i * 1103515245 + 12345) % 1000000;
    }

    quicksort(a, 0, n - 1);

    long chk = 0;
    for (long i = 0; i < n; i++) {
        chk = chk + a[i] * i;
    }
    printf("%ld\n", chk);

    delete[] a;
    return 0;
}
