#include "../tree.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <random>

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

TEST(LSMTreeTest, TestReadWrite1)
{
    LSMTree tree(2, 1, 10);

    auto key_values = GenKeyValues(100);
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    for (auto& kv : key_values) {
        std::string result;
        ASSERT_EQ(tree.Get(kv.first, result), true);
        ASSERT_EQ(result, kv.second);
    }
}

TEST(LSMTreeTest, TestReadWrite2)
{
    LSMTree tree(2, 3, 10);

    auto key_values = GenKeyValues(2000);
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    for (auto& kv : key_values) {
        std::string result;
        ASSERT_EQ(tree.Get(kv.first, result), true);
        ASSERT_EQ(result, kv.second);
    }
}

TEST(LSMTreeTest, TestDelete)
{
    LSMTree tree(2, 3, 10);

    auto key_values = GenKeyValues(1000);
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    for (auto& kv : key_values) {
        std::string result;
        tree.Delete(kv.first);
        ASSERT_EQ(tree.Get(kv.first, result), false);
    }
}

TEST(LSMTreeTest, TestQuery)
{
    LSMTree tree(2, 3, 10);

    auto key_values = GenKeyValues(1000);
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    sort(key_values.begin(), key_values.end());
    auto result = tree.GetQuery(key_values[300].first, key_values[500].first);
    std::cout << result.size() << std::endl;
    for (int i = 300; i < 501; ++i) {
        ASSERT_EQ(result[i - 300].second, key_values[i].second);
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
