#pragma once 

#include "memory_store.hpp"
#include <string>
#include <vector>

class JsonMemoryStore : public MemoryStore {
public:
    explicit JsonMemoryStore(const std::string &file_path);

    void add(const Memory &memory) override;
    std::vector<Memory> search(const std::vector<float> &embedding, size_t top_k) override;

private:
    std::string file_path;
    std::vector<Memory> memories;

    void load();
    void save() const;
};
