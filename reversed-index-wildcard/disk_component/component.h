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
    }

    void WriteToFile(KV& kv, FILE* file, bool is_tmp=false) {
        size_t val_bytes_size = kv.Value.getSizeInBytes();
        char val_bytes[val_bytes_size + 1];
        auto a = kv.Value.write(val_bytes);
        if (!is_tmp) {
            KVSizes_.emplace_back(val_bytes_size);
            KVSizesPrefixSum_.push_back(KVSizesPrefixSum_.back() + 4 + val_bytes_size);
        } else {
            KVSizesTmp_.emplace_back(val_bytes_size);
            KVSizesPrefixSumTmp_.push_back(KVSizesPrefixSumTmp_.back() + 4 + val_bytes_size);
        }
        fwrite(std::to_string(kv.Key).c_str(), sizeof(char), 4, file);
        fwrite(val_bytes, sizeof(char), val_bytes_size, file);
    }

    void ReadKeyFromFile(size_t index, KV& result, FILE* file) {
        size_t pos = KVSizesPrefixSum_[index];
        auto kvt_size = KVSizes_[index];
        fseek(file, pos, SEEK_SET);

        char buffer[kvt_size.KeySize + 1];
        fread(buffer, sizeof(char), kvt_size.KeySize, file);
        buffer[4] = '\0';
        result.Key = atoi(buffer);
    }

    void ReadFromFile(size_t index, KV& result, FILE* file) {
        size_t pos = KVSizesPrefixSum_[index];
        auto kvt_size = KVSizes_[index];
        ReadKeyFromFile(index, result, file);
        char buffer[kvt_size.ValueSize + 1];
        fread(buffer, sizeof(char), kvt_size.ValueSize, file);
        result.Value = roaring::Roaring::readSafe(buffer, kvt_size.ValueSize);
    }

    GetResult Get(K key) {
        if (KVSizes_.size() == 0) {
            return { false, V() };
        }
        FILE* file = fopen(DataFileName_.c_str(), "rb");
        size_t index = GetIndex(key, true, file);
        KV kvt;
        ReadFromFile(index, kvt, file);
        fclose(file);
        if (kvt.Key != key) {
            return { false, V() };
        }

        return { true, std::move(kvt.Value) };
    }

    /*void Delete(K key) {
        FILE* file = fopen(DataFileName_.c_str(), "rb");

        size_t index = GetIndex(key, true, file);
        KV kvt;
        ReadKeyFromFile(index, kvt, file);
        fclose(file);
        if (kvt.Key != key || kv) {
            return;
        }

        WriteTombstoneToFile(index);
    }*/

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
        KVSizes_ = {};
        KVSizesPrefixSum_ = { 0 };
        KVSizesTmp_ = {};
        KVSizesPrefixSumTmp_ = { 0 };
        Size_ = 0;
    }

    void SwapTmp() {
        KVSizes_ = KVSizesTmp_;
        KVSizesPrefixSum_ = KVSizesPrefixSumTmp_;
        KVSizesTmp_ = {};
        KVSizesPrefixSumTmp_ = { 0 };
        Size_ = KVSizes_.size();
    }

private:
    struct KVSize {
        size_t KeySize = 4;
        size_t ValueSize;

        KVSize(size_t value_size)
            : ValueSize(value_size)
        {}
    };

    size_t GetIndex(K key, bool is_first, FILE* file) {
        long long L = is_first ? -1 : 0;
        long long R = is_first ? KVSizes_.size() - 1 : KVSizes_.size();
        bool exists = false;
        while (R - L > 1) {
            size_t M = (L + R) / 2;
            KV kvt;
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


    size_t Size_ = 0;
    std::string DataFileName_;
    std::vector<KVSize> KVSizes_;
    std::vector<size_t> KVSizesPrefixSum_ = { 0 };
    std::vector<KVSize> KVSizesTmp_;
    std::vector<size_t> KVSizesPrefixSumTmp_ = { 0 };
};
