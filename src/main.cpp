#include "agent.hpp"
#include "llm_client.hpp"
#include "web_search_tool.hpp"

#include <iostream>
#include <curl/curl.h>

const std::string model = "qwen3.5:9b";

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    LlmClient client(model);
    WebSearchTool search;
    Agent agent(client, {&search});
    AgentResult result = agent.run("What is the captial of Australia?");

   if (result.output.has_value()) {
        if (const auto *output = std::get_if<Json>(&*result.output))
            std::cout << output->dump(4) << '\n';
        else 
            std::cout << std::get<std::string>(*result.output) << '\n';
   }

    curl_global_cleanup();
}
