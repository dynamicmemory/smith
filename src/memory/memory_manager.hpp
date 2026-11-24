#pragma once 

#include "memory_store.hpp"
#include "types.hpp"
#include "llm_client.hpp"
#include <string>
#include <vector>

inline constexpr const char *DEFAULT_MEMORY_EMBED_MODEL = "nomic-embed-text";


class MemoryManager { 
public:
    explicit MemoryManager(
            MemoryStore &store, 
            LlmClient &llm,
            const std::string &embedding_model = DEFAULT_MEMORY_EMBED_MODEL
    );

    void add_memory(Memory memory);
    std::vector<Memory> search_memories(const std::string &query, size_t top_k=5);
    void extract_memories(const std::vector<Event> &events);
    DuplicateCheckResult check_duplicate(const Memory &candidate, size_t top_k=5);

private:
    MemoryStore &store;
    LlmClient &llm;
    std::string embedding_model;
};
