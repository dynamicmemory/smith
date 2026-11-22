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
                "You are a helpful assistant",
                100,
                "agent",
                std::nullopt,
                {approval_callback},
                {search_compressor_callback},
                {optimizer_callback}
            );

    AgentResult result = agent.run("My name is bobby and im a programmer", "bobby");

    if (result.output.has_value()) {
        if (const auto *output = std::get_if<Json>(&*result.output))
            std::cout << output->dump(4) << '\n';
        else 
            std::cout << std::get<std::string>(*result.output) << '\n';
    }

    result = agent.run("Whats my name and whats my profession?", "bobby");

    if (result.output.has_value()) {
        if (const auto *output = std::get_if<Json>(&*result.output))
            std::cout << output->dump(4) << '\n';
        else 
            std::cout << std::get<std::string>(*result.output) << '\n';
    }

    curl_global_cleanup();
    return 0;
}
