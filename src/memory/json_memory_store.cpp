#include "json_memory_store.hpp"
#include "rag.hpp"
#include "types.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

JsonMemoryStore::JsonMemoryStore(const std::string &file_path) : file_path(file_path) {
    load();
}

void JsonMemoryStore::load() {
    std::ifstream file(file_path);

    // A missing file means we are starting weith a n empty store.
    if (!file.is_open())
        return;

    Json data;

    try { 
        file >> data;
    }
    catch (const Json::exception &e) {
        throw std::runtime_error("Failed to parse memory file: " + std::string(e.what()));
    }
    
    if (!data.is_array())
        throw std::runtime_error("Memory file must contain a JSON array");

    for (const auto &item : data) {
        Memory memory;

        memory.id = item.at("id").get<std::string>();
        memory.summary = item.at("summary").get<std::string>();
        memory.content= item.at("content").get<std::string>();
        memory.category= item.at("category").get<std::string>();
        memory.embedding = item.at("embedding").get<std::vector<float>>();

        memories.push_back(std::move(memory));
    }
}

void JsonMemoryStore::save() const {
    Json data = Json::array();

    for (const auto &memory : memories) {
        data.push_back({
            {"id", memory.id},
            {"summary", memory.summary},
            {"content", memory.content},
            {"category", memory.category},
            {"embedding", memory.embedding},
        });
    }

    std::ofstream file(file_path);

    if (!file.is_open())
        throw std::runtime_error("Could not open memory file for writing");

    file << data.dump(4);

    if (!file)
        throw std::runtime_error("Failed to write memory file");
}

void JsonMemoryStore::add(const Memory &memory) {
    memories.push_back(memory);
    save();
}

std::vector<Memory> JsonMemoryStore::search(const std::vector<float> &embedding,
                                            size_t top_k) {
    std::vector<std::pair<float, size_t>> scores;

    for (size_t i=0; i<memories.size(); ++i) {
        float similarity = cosine_similarities(embedding, memories[i].embedding);
        scores.push_back({similarity, i});
    }

    std::vector<Memory> results;
    top_k = std::min(top_k, scores.size());

    for (size_t i=0; i<top_k; ++i)
        results.push_back(memories[scores[i].second]);

    return results;
}
