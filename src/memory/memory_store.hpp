#pragma once 

#include "memory.hpp"
#include <vector> 

class MemoryStore {
public:
    virtual ~MemoryStore() = default;
    virtual void add(const Memory &memory) = 0;
    virtual std::vector<Memory> search(const std::vector<float> &embedding, size_t top_k) = 0;
};
