#include "../b_tree.h"

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

TEST(BTreeTest, TestAdd1)
{
    auto tree = BTree(2);
    std::string key = "abc";
    V value = "def";
    tree.Add(key, value);
    ASSERT_EQ(tree.Get(key).IsFound, true);
    ASSERT_EQ(tree.Get(key).Value, value);
}

TEST(BTreeTest, TestAdd1000)
{
    auto tree = BTree(5);
    auto key_values = GenKeyValues(1000);
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(key_values.begin(), key_values.end(), g);
    for (auto& kv : key_values) {
        ASSERT_EQ(tree.Get(kv.first).IsFound, true);
        ASSERT_EQ(tree.Get(kv.first).Value, kv.second);
    }
}

TEST(BTreeTest, TestDelete)
{
    auto tree = BTree(2);
    auto key_values = GenKeyValues(10);
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    for (auto& kv : key_values) {
        ASSERT_EQ(tree.Get(kv.first).IsFound, true);
        ASSERT_EQ(tree.Get(kv.first).Value, kv.second);
        tree.Delete(kv.first);
        ASSERT_EQ(tree.Get(kv.first).IsDeleted, true);
        tree.Add(kv.first, kv.second);
        ASSERT_EQ(tree.Get(kv.first).IsFound, true);
        ASSERT_EQ(tree.Get(kv.first).Value, kv.second);
        ASSERT_EQ(tree.Get(kv.first).IsDeleted, false);
    }
}

TEST(BTreeTest, TestGetQuery)
{
    auto tree = BTree(2);
    auto key_values = GenKeyValues(10);
    for (auto& kv : key_values) {
        tree.Add(kv.first, kv.second);
    }
    std::sort(key_values.begin(), key_values.end());
    auto result = tree.GetQuery(key_values[3].first, key_values[8].first);
    for (int i = 3; i < 9; ++i) {
        ASSERT_EQ(result[i - 3].Value, key_values[i].second);
    }
}

TEST(BTreeTest, TestList)
{
    auto tree = BTree(2);
    std::vector<std::pair<std::string, std::string>> keys_values = {
        { "a", "1" },
        { "b", "2" },
        { "c", "3" },
        { "d", "4" },
        { "e", "5" },
        { "f", "6" },
    };
    for (auto& kv : keys_values) {
        tree.Add(kv.first, kv.second);
    }
    auto list_result = tree.List();
    size_t index = 0;
    for (auto& elem : list_result) {
        KVTombstone exp_result = { keys_values[index].first, keys_values[index].second, false };
        ASSERT_EQ(elem.Key, exp_result.Key);
        ASSERT_EQ(elem.Value, exp_result.Value);
        ASSERT_EQ(elem.Tombstone, exp_result.Tombstone);
        ++index;
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
