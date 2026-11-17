// TODO: Rag currently depends directly on ollama. Intro an embedding provider 
// abstraction when supporting other providers.

#include "rag.hpp"
#include "ollama.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

// const std::string embed_model = "nomic-embed-text";

/* Call ollama using an embedding model and returns the JSON response*/
Json call_embedding_model(const std::vector<std::string> &text, 
                          const std::string &model) {

    Json payload = {
        {"model", model},
        {"input", text}
    };
     
    return ollama_post("http://localhost:11434/api/embed", payload);
}

//TODO: add optional for vector of strings of just string for text, use check to wrap string in a vector if needed
std::vector<std::vector<float>> get_embeddings(const std::vector<std::string> &text, 
                                               const std::string &model) {
    // Check on text 
    Json response = call_embedding_model(text, model);
    std::vector<std::vector<float>> embeddings;

    for (const auto &embedding : response.at("embeddings"))
        embeddings.push_back(embedding.get<std::vector<float>>());

    return embeddings;
}

float cosine_similarities(const std::vector<float> &a, const std::vector<float> &b ) {
    if (a.size() != b.size())
        throw std::invalid_argument("Embedding dimensions do not match");

    float dot_product = 0.0f;
    float mag_a = 0.0f;
    float mag_b = 0.0f;

    for (size_t i=0; i<a.size(); ++i) {
        dot_product += a[i] * b[i];
        mag_a += a[i] * a[i];
        mag_b += b[i] * b[i];
    }

    mag_a = std::sqrt(mag_a);
    mag_b = std::sqrt(mag_b);

    if (mag_a == 0.0f || mag_b == 0.0f)
        throw std::invalid_argument("Cannot calculate cosine similarity for zero vectors");

    return dot_product / (mag_a * mag_b);
}

std::vector<std::string> fixed_length_chunking(const std::string &text, int chunk_size, int overlap) {
    if (chunk_size <= 0 || overlap < 0) 
        throw std::invalid_argument("chunk_size must be > 0 and overlap >= 0");
    else if (overlap >= chunk_size)
        throw std::invalid_argument("chunk_size must be larger than overlap");

    std::vector<std::string> chunks;

    for (size_t start=0; start<text.size(); start += chunk_size - overlap) {
        chunks.push_back(text.substr(start, chunk_size));

        if ( (start + chunk_size) > text.size() )
            break;
    }
    return chunks;
}

// TODO: CUrrently hides away what model can be used.
std::vector<std::pair<float, std::string>> vector_search(
        const std::string &query,
        const std::vector<std::string> &chunks,
        const std::vector<std::vector<float>> &chunk_embeddings,
        size_t top_k
) {

    std::vector<std::vector<float>> query_embedding = get_embeddings({query});
    // TODO: If the book changes how we pass in the embed model, uncomment this line, delete above line
    // std::vector<std::vector<float>> query_embedding = get_embeddings({query}, embed_model);

    std::vector<std::pair<float, size_t>> scores;

    if (chunks.size() != chunk_embeddings.size())
        throw std::invalid_argument("chunks and chunk_embeddings must be the same size");

    if (query_embedding.size() == 0)
        throw std::invalid_argument("Vector search: embeddings returned an empty vector");

    size_t i = 0;
    for (const auto &chunk : chunk_embeddings) {
        float similarity = cosine_similarities(query_embedding[0], chunk);
        scores.push_back({similarity, i});

        ++i;
    }
    
    std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
        return a.first > b.first;
        }
    );

    std::vector<std::pair<float, std::string>> results;
    top_k = top_k > scores.size() ? scores.size() : top_k;

    for (size_t i=0; i<top_k; ++i)
        
        results.push_back({scores[i].first, chunks[scores[i].second]});
        
    return results;
}
