#include "../common/common.h"

#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <random>
#include <bitset>

class DiskComponent {
public:
    DiskComponent(std::string file_name)
        : DataFileName_(file_name)
    {
        std::random_device rd;
        std::mt19937 g(rd());
        HashSeeds_.resize(HASHES_LIST_LEN);
        for (size_t i = 0; i < HASHES_LIST_LEN; ++i) {
            HashSeeds_[i] = g() % 1000000;
        }
    }

    void WriteToFile(KVTombstone& kvt, FILE* file, bool is_tmp=false) {
        const char* val_bytes = kvt.Value.c_str();
        size_t val_bytes_size = kvt.Value.size();
        if (!is_tmp) {
            KVTSizes_.emplace_back(kvt.Key.size(), val_bytes_size);
            KVTSizesPrefixSum_.push_back(KVTSizesPrefixSum_.back() + kvt.Key.size() + val_bytes_size + 1);
            AddKey(kvt.Key);
        } else {
            KVTSizesTmp_.emplace_back(kvt.Key.size(), val_bytes_size);
            KVTSizesPrefixSumTmp_.push_back(KVTSizesPrefixSumTmp_.back() + kvt.Key.size() + val_bytes_size + 1);
            AddKeyTmp(kvt.Key);
        }
        char buffer[2];
        buffer[0] = kvt.Tombstone ? '1' : '0';
        fwrite(&buffer, sizeof(char), 1, file);
        fwrite(kvt.Key.c_str(), sizeof(char), kvt.Key.size(), file);
        fwrite(val_bytes, sizeof(char), val_bytes_size, file);
    }

    void ReadKeyFromFile(size_t index, KVTombstone& result, FILE* file) {
        size_t pos = KVTSizesPrefixSum_[index];
        auto kvt_size = KVTSizes_[index];
        fseek(file, pos, SEEK_SET);

        char tmp_buffer[2];
        fread(tmp_buffer, sizeof(char), 1, file);
        result.Tombstone = (tmp_buffer[0] == '1');

        char* buffer = new char[kvt_size.KeySize + 1];
        fread(buffer, sizeof(char), kvt_size.KeySize, file);
        result.Key = std::string(buffer, kvt_size.KeySize);
        delete[] buffer;
    }

    void ReadFromFile(size_t index, KVTombstone& result, FILE* file) {
        size_t pos = KVTSizesPrefixSum_[index];
        auto kvt_size = KVTSizes_[index];
        ReadKeyFromFile(index, result, file);
        char* buffer = new char[kvt_size.ValueSize + 1];
        fread(buffer, sizeof(char), kvt_size.ValueSize, file);
        result.Value = std::string(buffer, kvt_size.ValueSize);
        delete[] buffer;
    }

    GetResult Get(std::string& key) {
        if (!CheckKey(key)) {
            return { false, V(), false };
        }
        if (KVTSizes_.size() == 0) {
            return { false, V(), false };
        }
        FILE* file = fopen(DataFileName_.c_str(), "rb");
        size_t index = GetIndex(key, true, file);
        KVTombstone kvt;
        ReadFromFile(index, kvt, file);
        fclose(file);
        if (kvt.Key != key) {
            return { false, V(), false };
        }
        if (kvt.Tombstone) {
            return { true, V(), true };
        }

        return { true, std::move(kvt.Value), false };
    }

    void GetQuery(std::string& start_key, std::string& end_key, std::vector<KVTombstone>& result_values) {
        if (KVTSizes_.size() == 0) {
            return;
        }
        FILE* file = fopen(DataFileName_.c_str(), "rb");
        size_t start_index = GetIndex(start_key, true, file);
        size_t end_index = GetIndex(end_key, false, file);
        KVTombstone kvt;
        ReadKeyFromFile(start_index, kvt, file);
        if (kvt.Key < start_key || kvt.Key > end_key) {
            fclose(file);
            return;
        }
        for (size_t i = start_index; i < end_index + 1; ++i) {
            ReadFromFile(i, kvt, file);
            result_values.push_back(kvt);
        }
        fclose(file);
    }

    void Delete(std::string& key) {
        FILE* file = fopen(DataFileName_.c_str(), "rb");

        size_t index = GetIndex(key, true, file);
        KVTombstone kvt;
        ReadKeyFromFile(index, kvt, file);
        fclose(file);
        if (kvt.Key != key || kvt.Tombstone) {
            return;
        }

        WriteTombstoneToFile(index);
    }

    size_t GetSize() {
        return Size_;
    }

    void SetSize(size_t new_size) {
        Size_ = new_size;
    }

    void AddSize() {
        ++Size_;
    }

    void Erase() {
        KVTSizes_ = {};
        KVTSizesPrefixSum_ = { 0 };
        KVTSizesTmp_ = {};
        KVTSizesPrefixSumTmp_ = { 0 };
        Size_ = 0;
        FilterBits_ &= 0;
        FilterBitsTmp_ &= 0;
    }

    void SwapTmp() {
        KVTSizes_ = KVTSizesTmp_;
        KVTSizesPrefixSum_ = KVTSizesPrefixSumTmp_;
        KVTSizesTmp_ = {};
        KVTSizesPrefixSumTmp_ = { 0 };
        Size_ = KVTSizes_.size();
        FilterBits_ = FilterBitsTmp_;
        FilterBitsTmp_ &= 0;
    }

private:
    struct KVTSize {
        size_t KeySize;
        size_t ValueSize;
        size_t TmbSize = 1;

        KVTSize(size_t key_size, size_t value_size)
            : KeySize(key_size)
            , ValueSize(value_size)
        {}
    };

    size_t GetIndex(std::string& key, bool is_first, FILE* file) {
        long long L = is_first ? -1 : 0;
        long long R = is_first ? KVTSizes_.size() - 1 : KVTSizes_.size();
        bool exists = false;
        while (R - L > 1) {
            size_t M = (L + R) / 2;
            KVTombstone kvt;
            ReadKeyFromFile(M, kvt, file);
            if (is_first) {
                if (kvt.Key < key) {
                    L = M;
                } else {
                    R = M;
                }
            } else {
                if (kvt.Key <= key) {
                    L = M;
                } else {
                    R = M;
                }
            }
        }
        return is_first ? R : L;
    }

    void WriteTombstoneToFile(size_t index) {
        FILE* file = fopen(DataFileName_.c_str(), "rb+");
        size_t pos = KVTSizesPrefixSum_[index];
        auto kvt_size = KVTSizes_[index];
        fseek(file, pos, SEEK_SET);

        char buffer[2];
        buffer[0] = '1';
        fwrite(&buffer, sizeof(char), 1, file);
        fclose(file);
    }

    void AddKey(std::string& key) {
        for (size_t i = 0; i < HASHES_LIST_LEN; ++i) {
            auto hash_result = std::hash<std::string>{}(key.c_str()) * HashSeeds_[i] % FILTER_BITS_LEN;
            FilterBits_.set(hash_result);
        }
    }

    void AddKeyTmp(std::string& key) {
        for (size_t i = 0; i < HASHES_LIST_LEN; ++i) {
            auto hash_result = std::hash<std::string>{}(key.c_str()) * HashSeeds_[i] % FILTER_BITS_LEN;
            FilterBitsTmp_.set(hash_result);
        }
    }

    bool CheckKey(std::string& key) {
        for (size_t i = 0; i < HASHES_LIST_LEN; ++i) {
            auto hash_result = std::hash<std::string>{}(key.c_str()) * HashSeeds_[i] % FILTER_BITS_LEN;
            if (!FilterBits_[hash_result]) {
                return false;
            }
        }
        return true;
    }

    const size_t HASHES_LIST_LEN = 10;
    const size_t FILTER_BITS_LEN = 32 * 1024;

    size_t Size_ = 0;
    std::string DataFileName_;
    std::vector<KVTSize> KVTSizes_;
    std::vector<size_t> KVTSizesPrefixSum_ = { 0 };
    std::vector<KVTSize> KVTSizesTmp_;
    std::vector<size_t> KVTSizesPrefixSumTmp_ = { 0 };

    std::vector<int> HashSeeds_;
    std::bitset<32 * 1024> FilterBits_;
    std::bitset<32 * 1024> FilterBitsTmp_;
};
