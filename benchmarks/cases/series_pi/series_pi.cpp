#include <cstdio>

int main() {
    long n = 10000000;
    double sum = 0.0;
    double sign = 1.0;

    for (long k = 0; k < n; k++) {
        double denom = 2.0 * k + 1.0;
        sum = sum + sign / denom;
        sign = -sign;
    }

    double pi = sum * 4.0;
    printf("%lf\n", pi);
    return 0;
}
