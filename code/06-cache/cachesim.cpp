#include <cstdio>
#include <cstdint>
#include <vector>

int main() {
    const int E = 12;                 // 12 路
    std::vector<std::vector<uint32_t>> sets(64);

    long long hits = 0, misses = 0, evictions = 0;

    // 先不读文件，用程序自己生成的地址序列测试
    //const uint32_t test_addrs[] = {0, 4096, 8192, 12288, 16384, 20480, 24576, 28672, 32768, 36864, 40960, 45056,
// 0, 49152, 0, 4096};
    std::vector<uint32_t> addrs;

    for (int tag = 0; tag < 64;tag++) {
        addrs.push_back(tag * 4096 + (tag % 64) * 64);
    }

    for (int tag = 0; tag < 64;tag++) {
        addrs.push_back(tag * 4096 + (tag % 64) * 64);
    }

    for (uint32_t addr : addrs) {
        uint32_t set = (addr / 64) % 64;
        uint32_t tag = addr / 4096;

        // ********************************************
        // 你写这里：
        //   在 sets[set] 里找 tag
        //   找到   -> hits++, 挪到队尾
        //   没找到 -> misses++
        //             如果 sets[set].size() == E，删队首, evictions++
        //             push_back(tag)
        // ********************************************
        int pos = -1;
        for (int idx = 0;idx < (int)sets[set].size();idx++) {
            if ((int)sets[set][idx] == tag) {
                pos = idx;
                hits++;
                sets[set].erase(sets[set].begin() + idx);
                sets[set].push_back(tag);
                break;
            }
        }
        if (pos == -1) {
            misses++;
            if (sets[set].size() == E) {
                sets[set].erase(sets[set].begin());
                evictions++;
            }
            sets[set].push_back(tag);
        }
    }

    printf("hits      = %lld\n", hits);
    printf("misses    = %lld\n", misses);
    printf("evictions = %lld\n", evictions);
    return 0;
}
