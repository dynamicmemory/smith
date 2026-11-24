#pragma once 

#include <string>
#include <vector>

struct Memory {
    std::string id;
    std::string summary;
    std::string content;
    std::string category;
    std::vector<float> embedding;
};

enum class MemoryDecision {
    add, skip 
};

struct DuplicateCheckResult {
    MemoryDecision decision;
    std::string reason;
};
