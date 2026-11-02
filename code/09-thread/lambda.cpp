#include <cstdio>
#include <vector>
#include <algorithm>

int main() {
    auto hello = [] {printf("hello\n");};
    hello();

    auto add = [](int a, int b) {return a + b;};
    printf("%d\n", add(3, 4));

    int x = 10;
    auto byValue = [x] {return x * 2;};
    x = 999;
    printf("byValue=%d\n", byValue());

    auto byRef = [&x] {return x * 2;};
    printf("byRef=%d\n", byRef()); 

    auto divide = [](double a, double b) -> double {
        if (b == 0.0) return 0.0;
        return a / b;
    };
    printf("%f\n", divide(7.0, 2.0)); 

    std::vector<int> v = {3, 1, 4, 1, 5};
    std::sort(v.begin(), v.end(),[](int a, int b) { return  a > b;});
    for (int n : v) printf("%d ", n);           // 5 4 3 1 1
    printf("\n");
    return 0;
}