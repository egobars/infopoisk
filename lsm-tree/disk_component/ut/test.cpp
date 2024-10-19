#include "../component.h"

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

TEST(DiskComponentTest, TestReadWrite)
{
    KVTombstone data;
    data.Key = "abc";
    data.Value = "def";
    data.Tombstone = true;
    DiskComponent cmp("");

    FILE* file = fopen("tmp.txt", "wb");
    cmp.WriteToFile(data, file);
    fclose(file);

    KVTombstone new_data;
    file = fopen("tmp.txt", "rb");
    cmp.ReadFromFile(0, new_data, file);
    fclose(file);

    ASSERT_EQ(new_data.Key, data.Key);
    ASSERT_EQ(new_data.Value, data.Value);
    ASSERT_EQ(new_data.Tombstone, data.Tombstone);
}

TEST(DiskComponentTest, TestReadWrite2)
{
    KVTombstone data;
    data.Key = "abc";
    data.Value = "def";
    data.Tombstone = true;
    DiskComponent cmp("");

    FILE* file = fopen("tmp.txt", "wb");
    cmp.WriteToFile(data, file);
    data.Key = "uuuu";
    data.Value = "cccc";
    cmp.WriteToFile(data, file);
    data.Key = "ooo";
    data.Value = "kkk";
    cmp.WriteToFile(data, file);
    fclose(file);

    KVTombstone new_data;
    file = fopen("tmp.txt", "rb");
    cmp.ReadFromFile(1, new_data, file);
    fclose(file);

    ASSERT_EQ(new_data.Key, "uuuu");
    ASSERT_EQ(new_data.Value, "cccc");
    ASSERT_EQ(new_data.Tombstone, true);
}

TEST(DiskComponentTest, TestGet)
{
    FILE* file = fopen("tmp.txt", "wb");
    DiskComponent cmp("tmp.txt");
    auto key_values = GenKeyValues(10);
    sort(key_values.begin(), key_values.end());
    for (auto& kv : key_values) {
        KVTombstone data = {
            kv.first,
            kv.second,
            false
        };
        cmp.WriteToFile(data, file);
    }
    fclose(file);
    for (auto& kv : key_values) {
        ASSERT_EQ(cmp.Get(kv.first).IsFound, true);
        ASSERT_EQ(cmp.Get(kv.first).Value, kv.second);
    }
    std::string non_existing_key = "abcd";
    ASSERT_EQ(cmp.Get(non_existing_key).IsFound, false);
}


TEST(DiskComponentTest, TestDelete)
{
    FILE* file = fopen("tmp.txt", "wb");
    DiskComponent cmp("tmp.txt");
    auto key_values = GenKeyValues(10);
    sort(key_values.begin(), key_values.end());
    for (auto& kv : key_values) {
        KVTombstone data = {
            kv.first,
            kv.second,
            false
        };
        cmp.WriteToFile(data, file);
    }
    fclose(file);
    cmp.Delete(key_values[7].first);
    std::string result;
    ASSERT_EQ(cmp.Get(key_values[7].first).IsDeleted, true);
    ASSERT_EQ(cmp.Get(key_values[5].first).IsDeleted, false);
}

TEST(DiskComponentTest, TestQuery)
{
    FILE* file = fopen("tmp.txt", "wb");
    DiskComponent cmp("tmp.txt");
    auto key_values = GenKeyValues(10);
    sort(key_values.begin(), key_values.end());
    for (auto& kv : key_values) {
        KVTombstone data = {
            kv.first,
            kv.second,
            false
        };
        cmp.WriteToFile(data, file);
    }
    fclose(file);
    std::vector<KVTombstone> result;
    cmp.GetQuery(key_values[3].first, key_values[8].first, result);
    for (int i = 3; i < 9; ++i) {
        ASSERT_EQ(result[i - 3].Value, key_values[i].second);
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
