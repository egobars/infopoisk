#pragma once

#include <string>
#include <vector>
#include <../roaring/roaring.hh>

using K = uint32_t;
using V = roaring::Roaring;

struct GetResult {
    bool IsFound = false;
    V Value;

    GetResult(bool isFound, V value)
        : IsFound(isFound)
        , Value(value)
    {}
};

struct KV {
    K Key = 0;
    V Value = V();

    KV() {}

    KV(K key, V& value)
        : Key(key)
        , Value(value)
    {}
};
