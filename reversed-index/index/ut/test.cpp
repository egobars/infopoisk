#include "../index.h"

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

TEST(IndexTest, TestReadWrite1)
{
    Index tree(2, 1, 10);
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
        V result;
        tree.Get(kv.first, result);
        ASSERT_EQ(result, kv.second);
    }
}

TEST(IndexTest, TestReadWrite2)
{
    Index tree(2, 3, 10);

    auto values = GenValues(2000);
    std::vector<std::pair<K, V>> to_add_kv;
    std::vector<std::pair<K, V>> key_values;
    for (unsigned int i = 0; i < 500; ++i) {
        to_add_kv.emplace_back(i, values[i]);
        to_add_kv.emplace_back(i, values[i + 500]);
        to_add_kv.emplace_back(i, values[i + 1000]);
        to_add_kv.emplace_back(i, values[i + 1500]);
        auto to_add = values[i] | values[i + 500] | values[i + 1000] | values[i + 1500];
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
        V result;
        tree.Get(kv.first, result);
        ASSERT_EQ(result, kv.second);
    }
}

TEST(IndexTest, TestDocument)
{
    Index index(2, 3, 10);
    index.AddDocument("cat", "i have cats, and dog, and horse");
    index.AddDocument("dog", "i have Dog. and Horse and Crocodile");
    index.AddDocument("croco", "i have cat!! and dog!!! and crocodile!");
    {
        Finder finder(index, "cat");
        std::vector<std::string> expected = { "cat", "croco" };
        ASSERT_EQ(finder.GetDocuments(), expected);
    }
    {
        Finder finder(index, "cat");
        std::vector<std::string> expected = { "cat" };
        ASSERT_EQ(finder.And("horse").GetDocuments(), expected);
    }
    {
        Finder finder(index, "horse");
        std::vector<std::string> expected = { "cat", "dog", "croco" };
        ASSERT_EQ(finder.Or("crocodile").GetDocuments(), expected);
    }
    {
        Finder finder(index, "cat");
        std::vector<std::string> expected = { "croco" };
        ASSERT_EQ(finder.Or("dog").Without("horse").GetDocuments(), expected);
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
