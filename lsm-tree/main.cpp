#include "lsm-tree/tree.h"

#include <ctime>
#include <random>
#include <algorithm>

std::string GenString(size_t len) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::string result;
    for (size_t i = 0; i < len; ++i) {
        result += 'a' + g() % 27;
    }
    return result;
}

std::vector<std::pair<std::string, std::string>> GenKeyValues(size_t n) {
    size_t len = 10;
    std::set<std::string> keys;
    std::vector<std::pair<std::string, std::string>> result;
    while (keys.size() < n) {
        std::string new_key = GenString(len);
        if (keys.find(new_key) == keys.end()) {
            keys.insert(new_key);
            result.emplace_back(new_key, GenString(len));
        }
    }
    return result;
}

int main() {
    std::random_device rd;
    std::mt19937 g(rd());

    LSMTree tree(2, 10, 10);

    size_t SIZE = 1000000;

    auto key_values = GenKeyValues(SIZE);
    std::cout << "GENERATION END" << std::endl;

    clock_t timestamp_start = clock();
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    clock_t delta = clock() - timestamp_start;
    std::cout << "ADD TIME: " << (double) delta / CLOCKS_PER_SEC << " sec" << std::endl;

    std::shuffle(key_values.begin(), key_values.end(), g);
    std::cout << "SHUFFLE END" << std::endl;

    timestamp_start = clock();
    for (auto& kv : key_values) {
        std::string result;
        tree.Get(kv.first, result);
    }
    delta = clock() - timestamp_start;
    std::cout << "GET TIME: " << (double) delta / CLOCKS_PER_SEC << " sec" << std::endl;

    std::sort(key_values.begin(), key_values.end());
    std::cout << "SORT END" << std::endl;

    timestamp_start = clock();
    for (size_t i = 0; i < SIZE; ++i) {
        size_t first_idx = g() % (SIZE - 5);
        tree.GetQuery(key_values[first_idx].first, key_values[first_idx + 5].first);
    }
    delta = clock() - timestamp_start;
    std::cout << "GET QUERY TIME: " << (double) delta / CLOCKS_PER_SEC << " sec" << std::endl;
    return 0;
}
