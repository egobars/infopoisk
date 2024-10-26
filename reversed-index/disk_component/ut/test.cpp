#include "../component.h"

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

TEST(DiskComponentTest, TestReadWrite)
{
    KV data;
    data.Key = 5;
    data.Value = roaring::Roaring();
    data.Value.add(3);
    data.Value.add(5);
    data.Value.add(7);
    DiskComponent cmp("");

    FILE* file = fopen("tmp.txt", "wb");
    cmp.WriteToFile(data, file);
    fclose(file);

    KV new_data;
    file = fopen("tmp.txt", "rb");
    cmp.ReadFromFile(0, new_data, file);
    fclose(file);

    ASSERT_EQ(new_data.Key, data.Key);
    ASSERT_EQ(new_data.Value, data.Value);
}

TEST(DiskComponentTest, TestReadWrite2)
{
    KV data;
    data.Key = 5;
    data.Value = { 5, 6, 7 };
    DiskComponent cmp("");

    FILE* file = fopen("tmp.txt", "wb");
    cmp.WriteToFile(data, file);
    data.Key = 7;
    data.Value = { 3, 4, 5 };
    cmp.WriteToFile(data, file);
    data.Key = 9;
    data.Value = { 7, 12, 154 };
    cmp.WriteToFile(data, file);
    fclose(file);

    KV new_data;
    file = fopen("tmp.txt", "rb");
    cmp.ReadFromFile(1, new_data, file);
    fclose(file);

    auto good_value = roaring::Roaring({ 3, 4, 5 });
    ASSERT_EQ(new_data.Key, 7);
    ASSERT_EQ(new_data.Value, good_value);
}

TEST(DiskComponentTest, TestGet)
{
    FILE* file = fopen("tmp.txt", "wb");
    DiskComponent cmp("tmp.txt");
    auto values = GenValues(1000);
    std::vector<std::pair<K, V>> key_values;
    for (unsigned int i = 0; i < 250; ++i) {
        auto to_add = values[i] | values[i + 250] | values[i + 500] | values[i + 750];
        key_values.emplace_back(i, to_add);
    }
    for (auto& kv : key_values) {
        KV data = {
            kv.first,
            kv.second,
        };
        cmp.WriteToFile(data, file);
    }
    fclose(file);
    for (auto& kv : key_values) {
        ASSERT_EQ(cmp.Get(kv.first).IsFound, true);
        ASSERT_EQ(cmp.Get(kv.first).Value, kv.second);
    }
    K non_existing_key = 1234;
    ASSERT_EQ(cmp.Get(non_existing_key).IsFound, false);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
