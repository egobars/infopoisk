#pragma once

#include <string>
#include <vector>

using K = std::string;
using V = std::string;

struct GetResult {
    bool IsFound = false;
    V Value;
    bool IsDeleted = false;
};

struct KVTombstone {
    std::string Key = "";
    V Value = V();
    bool Tombstone = false;

    KVTombstone() {}

    KVTombstone(std::string& key, V& value, bool tombstone)
        : Key(key)
        , Value(value)
        , Tombstone(tombstone)
    {}
};
