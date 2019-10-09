#define N 32768
void demo(double* __restrict c, double* a) {
    for (unsigned i = 0; i<N; ++i) {
        c[i] = a[i]*a[i];
    }
}
