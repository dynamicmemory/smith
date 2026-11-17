#pragma once

#include "types.hpp"
#include <string>
#include <vector>
#include <utility>

// TODO: Rag currently depends directly on ollama. Intro an embedding provider 
// abstraction when supporting other providers.

inline constexpr const char *DEFAULT_EMBED_MODEL = "nomic-embed-text";

/* Call ollama using an embedding model and returns the JSON response*/
Json call_embedding_model(
        const std::vector<std::string> &text, 
        const std::string &model = DEFAULT_EMBED_MODEL
);

//TODO: add optional for vector of strings of just string for text, use check to wrap string in a vector if needed
std::vector<std::vector<float>> get_embeddings(
        const std::vector<std::string> &text, 
        const std::string &model = DEFAULT_EMBED_MODEL
);

float cosine_similarities(const std::vector<float> &a, const std::vector<float> &b );

std::vector<std::string> fixed_length_chunking(const std::string &text, int chunk_size, int overlap);

std::vector<std::pair<float, std::string>> vector_search(
        const std::string &query,
        const std::vector<std::string> &chunks,
        const std::vector<std::vector<float>> &chunk_embeddings,
        size_t top_k
);
