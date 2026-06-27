#include <cstdio>

int main() {
    long W = 600;
    long H = 600;
    long maxIter = 256;
    long total = 0;

    for (long py = 0; py < H; py++) {
        for (long px = 0; px < W; px++) {
            double x0 = px * 1.0 / W * 3.5 - 2.5;
            double y0 = py * 1.0 / H * 2.0 - 1.0;
            double x = 0.0;
            double y = 0.0;
            long iter = 0;
            while (iter < maxIter && x * x + y * y < 4.0) {
                double xt = x * x - y * y + x0;
                y = 2.0 * x * y + y0;
                x = xt;
                iter++;
            }
            total = total + iter;
        }
    }

    printf("%ld\n", total);
    return 0;
}
