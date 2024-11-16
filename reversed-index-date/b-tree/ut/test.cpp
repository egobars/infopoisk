#include "../b_tree.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <random>

V GenV(size_t len) {
    std::random_device rd;
    std::mt19937 g(rd());
    auto result = roaring::Roaring();
    for (size_t i = 0; i < len; ++i) {
        result.add(g() % 1000);
    }
    return result;
}

std::vector<V> GenValues(size_t n) {
    size_t len = 10;
    std::vector<V> result;
    for (size_t i = 0; i < n; ++i) {
        result.push_back(GenV(len));
    }
    return result;
}

TEST(BTreeTest, TestAdd1)
{
    auto tree = BTree(2);
    K key = 5;
    V value = roaring::Roaring();
    value.add(3);
    value.add(5);
    value.add(7);
    tree.Add(key, value);
    V value2 = roaring::Roaring();
    value2.add(9);
    value2.remove(3);
    tree.Add(key, value2);
    ASSERT_EQ(tree.Get(key).IsFound, true);
    ASSERT_EQ(tree.Get(key).Value, value | value2);
}

TEST(BTreeTest, TestAdd1000)
{
    auto tree = BTree(5);
    auto values = GenValues(1000);
    std::vector<std::pair<K, V>> to_add_kv;
    std::vector<std::pair<K, V>> key_values;
    for (unsigned int i = 0; i < 250; ++i) {
        to_add_kv.emplace_back(i, values[i]);
        to_add_kv.emplace_back(i, values[i + 250]);
        to_add_kv.emplace_back(i, values[i + 500]);
        to_add_kv.emplace_back(i, values[i + 750]);
        auto to_add = values[i] | values[i + 250] | values[i + 500] | values[i + 750];
        key_values.emplace_back(i, to_add);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(to_add_kv.begin(), to_add_kv.end(), g);
    for (auto& kv : to_add_kv) {
        tree.Add(kv.first, kv.second);
    }
    std::shuffle(key_values.begin(), key_values.end(), g);
    for (auto& kv : key_values) {
        ASSERT_EQ(tree.Get(kv.first).IsFound, true);
        ASSERT_EQ(tree.Get(kv.first).Value, kv.second);
    }
}

TEST(BTreeTest, TestList)
{
    auto tree = BTree(2);
    auto values = GenValues(20);
    std::vector<std::pair<K, V>> key_values;
    for (unsigned int i = 0; i < 5; ++i) {
        auto to_add = values[i] | values[i + 5] | values[i + 10] | values[i + 15];
        tree.Add(i, to_add);
        key_values.emplace_back(i, to_add);
    }
    auto list_result = tree.List();
    size_t index = 0;
    for (auto& elem : list_result) {
        KV exp_result = { key_values[index].first, key_values[index].second };
        ASSERT_EQ(elem.Key, exp_result.Key);
        ASSERT_EQ(elem.Value, exp_result.Value);
        ++index;
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
