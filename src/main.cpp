#include "types.hpp"
#include "agent.hpp"
#include "llm_client.hpp"
#include "ollama.hpp"
#include "rag.hpp"
#include "approval_callback.hpp"
#include "search_compressor_callback.hpp"

#include "web_search_tool.hpp"
#include "list_files_tool.hpp"
#include "read_file_tool.hpp"
#include "create_file_tool.hpp"
#include "delete_file_tool.hpp"

#include "context_optimizer.hpp"

#include <curl/curl.h>
#include <iostream>

const std::string model = "qwen3.5:9b";
const std::string embed_model = "nomic-embed-text";

int main(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    LlmClient client(model);
    ListFilesTool ls;
    ReadFileTool rd;
    CreateFileTool cf;
    DeleteFileTool df;
    WebSearchTool search;

    ContextOptimizer optimizer(client);

    BeforeLlmCallback optimizer_callback = 
        [&optimizer](ExecutionContext &context, LlmRequest &request) {
            return optimizer(context, request);
        };

    Agent agent(client, 
                {&ls, &rd, &cf, &df, &search}, 
                "You are a helpful assistant that can explore and manage files.",
                100,
                "agent deletor",
                std::nullopt,
                {approval_callback},
                {search_compressor_callback},
                {optimizer_callback}
            );

    ExecutionContext context;
    AgentResult result = agent.run("Get me todays date, the current price of bitcoin", &context);

    if (result.output.has_value()) {
        if (const auto *output = std::get_if<Json>(&*result.output))
            std::cout << output->dump(4) << '\n';
        else 
            std::cout << std::get<std::string>(*result.output) << '\n';
    }

    curl_global_cleanup();
    return 0;
}


// int main(void) {
//     curl_global_init(CURL_GLOBAL_DEFAULT);
//
//     std::vector<std::string> documents = {
//         {"The cat is sleeping on the couch"},
//         {"A kitten is playing with a toy"},
//         {"The dog is running in the park"},
//         {"Python is a programming language"},
//         {"Machine learning uses Python extensively"},
//         {"Cats are popular pets"},
//         {"Deep learning is a subset of machine learning"},
//     };
//
//     std::vector<std::vector<float>> doc_embeddings = get_embeddings(documents, embed_model);
//     std::vector<std::pair<float, std::string>> results = vector_search("Artificial Intelligence",
//                                                                        documents, 
//                                                                        doc_embeddings,
//                                                                        4);
//     for (const auto &result : results) 
//         std::cout << result.first << ": " << result.second << '\n';
//
//     LlmClient client(model);
//     ListFilesTool ls;
//     ReadFileTool rd;
//
//     WebSearchTool search;
//     Agent agent(client, {&ls, &rd, &search});
//     AgentResult result = agent.run("List the files and directories in the current project root directory."
//                                     "then find agent.cpp which is somewhere in the project, "
//                                     "read the file and give me a brief explanation of "
//                                     "what it does");
//
//     if (result.output.has_value()) {
//         if (const auto *output = std::get_if<Json>(&*result.output))
//             std::cout << output->dump(4) << '\n';
//         else 
//             std::cout << std::get<std::string>(*result.output) << '\n';
//     }
//
//     curl_global_cleanup();
//     return 0;
// }
//
