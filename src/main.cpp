#include <string>
#include <vector>
#include "types.hpp"

Json create_embeddings(std::vector<std::string> text, 
                       const std::string &model) {
    // Implement the call to ollama to get the embedding vector
    Json response;
    return response
}


std::vector<float> get_embeddings(std::vector<std::string> text, 
                                        const std::string &model) {

    // Check on text 

    Json response = create_embeddings(text, model);
    std::vector<float> embeddings;

    for (const auto &embed : response)
        embeddings.push_back(embed);

    return embeddings;
}


std::vector<std::string> fixed_length_chunking(std::vector<std::string> text, 
                                               int chunk_size,
                                               int overlap) {

}

std::vector<std::string> vector_search(const std::string &query,
                                       const std::vector<std::string> &chunks,
                                       const std::vector<std::string> &chunk_embeddings,
                                       int top_k) {

}

void unzip_file_tool();
void list_files_tool();
void read_file_tool();
void read_media_file_tool();


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
