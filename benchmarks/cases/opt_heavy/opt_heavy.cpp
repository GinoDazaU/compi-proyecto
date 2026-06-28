#include <cstdio>

int main() {
    long sum = 0;
    for (long i = 0; i < 100000000; i++) {
        long a = 1;
        long b = 0;
        long c = 1;
        long d = 0;
        long val = (((i * a + b) * c) + d);
        if (1 == 0) {
            sum = sum - 9999;
        } else {
            if (0 == 1) {
                sum = sum - 1111;
            } else {
                sum = sum + val;
            }
        }
    }
    printf("%ld\n", sum);
    return 0;
}
