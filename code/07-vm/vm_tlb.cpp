#include <cstdio>
#include <chrono>

int main() {
    const size_t MAXB = 256ull*1024*1024;
    char *p = new char[MAXB];

    for (size_t i = 0;i < MAXB;i += 4096) {
        p[i] = 1;
    }


    for (size_t wc = 64 * 1024;wc <= MAXB;wc *= 2) {
        size_t pages = wc / 4096;
        size_t mask = pages - 1;
        size_t page = 0;
        long long sum = 0;
        
        auto t0 = std::chrono::steady_clock::now();
        for (int i = 0;i < 1000000;i++) {
            page = (page + 12345) & mask;
            sum += p[page * 4096];
        }
        auto t1 = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / 1000000.0;
        printf("wc = %5zu KB pages = %6zu %.2f ns/access sum = %lld\n", wc / 1024, pages, ns, sum);

    }

    return 0;
}