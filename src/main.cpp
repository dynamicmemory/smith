#include <string>
#include <vector>
#include "types.hpp"
#include "ollama.hpp"
#include <curl/curl.h>
#include <cmath>

#include <iostream>

const std::string model = "qwen3.5:9b";
const std::string embed_model = "nomic-embed-text";

/* Call ollama using an embedding model and returns the JSON response*/
Json call_embedding_model(std::vector<std::string> text, 
                       const std::string &model) {

    Json payload = {
        {"model", model},
        {"input", text}
    };
     
    return ollama_post("http://localhost:11434/api/embed", payload);
}


std::vector<std::vector<float>> get_embeddings(std::vector<std::string> text, 
                                               const std::string &model) {
    // Check on text 
    Json response = call_embedding_model(text, model);
    std::vector<std::vector<float>> embeddings;


    for (const auto &embedding : response.at("embeddings"))
        embeddings.push_back(embedding.get<std::vector<float>>());

    return embeddings;
}

float who_sign_you_sign_cosine(const std::vector<float> &a, const std::vector<float> &b ) {
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
    if (chunk_size <= 0 || overlap <= 0) 
        throw std::invalid_argument("chunk_size and overlap must be greater than 0");
    else if (overlap > chunk_size)
        throw std::invalid_argument("chunk_size must be larger than overlap");

    std::vector<std::string> chunks;

    for (size_t start=0; start<text.size(); start += chunk_size - overlap) {
        chunks.push_back(text.substr(start, chunk_size));

        if ( (start + chunk_size) > text.size() )
            break;
    }
    return chunks;
}

//
// std::vector<std::string> vector_search(const std::string &query,
//                                        const std::vector<std::string> &chunks,
//                                        const std::vector<std::string> &chunk_embeddings,
//                                        int top_k) {
//
// }

void unzip_file_tool();
void list_files_tool();
void read_file_tool();
void read_media_file_tool();

int main(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::vector<std::string> sentences = {
        {"The cat is sleeping on the couch"},
        {"A kitten is playing with a toy"},
        {"The dog is running in the park"}
    };

    std::vector<std::vector<float>> embeddings = get_embeddings(sentences, embed_model);

    float cat_kitten = who_sign_you_sign_cosine(embeddings[0], embeddings[1]);
    float cat_dog = who_sign_you_sign_cosine(embeddings[0], embeddings[2]);
    float cat_cat = who_sign_you_sign_cosine(embeddings[0], embeddings[0]);
    
    std::cout << cat_kitten << '\n'; 
    std::cout << cat_dog << '\n'; 
    std::cout << cat_cat << '\n'; 

    std::string alpha = "abcdefghijklmnopqrstuvwxyz";
    std::vector<std::string> chunks = fixed_length_chunking(alpha, 10, 1);
    std::vector<std::string> chunks2 = fixed_length_chunking(alpha, 10, 5);

    std::cout << "Original: " << alpha.size() << " chars -> " << chunks.size() << " chunks" << '\n';
    std::cout << "Original: " << alpha.size() << " chars -> " << chunks2.size() << " chunks" << '\n';

    for (auto chunk : chunks2)
        std::cout << chunk << '\n';
    curl_global_cleanup();
    return 0;
}

// #include "agent.hpp"
// #include "llm_client.hpp"
// #include "web_search_tool.hpp"
//
// #include <iostream>
// #include <curl/curl.h>
//
// const std::string model = "qwen3.5:9b";
//
// int main() {
//     curl_global_init(CURL_GLOBAL_DEFAULT);
//
//     LlmClient client(model);
//     WebSearchTool search;
//     Agent agent(client, {&search});
//     AgentResult result = agent.run("What is the captial of Australia?");
//
//    if (result.output.has_value()) {
//         if (const auto *output = std::get_if<Json>(&*result.output))
//             std::cout << output->dump(4) << '\n';
//         else 
//             std::cout << std::get<std::string>(*result.output) << '\n';
//    }
//
//     curl_global_cleanup();
// }
