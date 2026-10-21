#include <cstdio>
#include <chrono>
#include <vector>
#include <cstdlib>

int main() {
    const int N = 2112;

    std::vector<int> a(N * N), b(N * N);

    for (int i = 0;i < N * N;i++) {
        a[i] = rand() & 0xFF;
    }

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0;i < N;i++) {
        for (int j = 0;j < N;j++) {
            b[j * N + i] = a[i * N + j];
        }
    }
    auto t1 = std::chrono::steady_clock::now();

    double ns1 = std::chrono::duration<double, std::nano>(t1 - t0).count();
    
    bool ok = true;
    for (int j = 0; j < N && ok; j++) {
        for (int i = 0; i < N; i++) {
            if (b[j * N + i] != a[i * N + j]) { ok = false; break; }
        }
    }
    printf("%s\n", ok ? "OK" : "FAIL");


    printf("naive   : %.3f ms\n", ns1 / 1e6);

    for (int B = 4; B <= 64; B *= 2) {
        auto s0 = std::chrono::steady_clock::now();

        for (int ii = 0; ii < N; ii += B)
            for (int jj = 0; jj < N; jj += B)
                for (int i = ii; i < ii + B; i++)
                    for (int j = jj; j < jj + B; j++)
                        b[j * N + i] = a[i * N + j];

        auto s1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(s1 - s0).count();
        printf("B = %2d   %8.3f ms\n", B, ms);
    }

    ok = true;
    for (int j = 0; j < N && ok; j++) {
        for (int i = 0; i < N; i++) {
            if (b[j * N + i] != a[i * N + j]) { ok = false; break; }
        }
    }
    printf("%s\n", ok ? "OK" : "FAIL");
    return 0;
}
