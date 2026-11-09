#include <curl/curl.h>
#include "types.hpp"
#include "llm_client.hpp"
#include "web_search_tool.hpp"
#include "agent.hpp"
#include <iostream> 

std::string model = "qwen3.5:9b";

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    LlmClient client(model);
    WebSearchTool web_search;

    std::vector<BaseTool*> tools = {&web_search};

    Agent agent(client, tools, "Use tools when needed instead of guessing and making, up answer", 10);

    AgentResult result = agent.run("What is the date today? and what is the current price of bitcoin");

    if (result.output.has_value()) {
        if (const auto *text = std::get_if<std::string>(&*result.output))
            std::cout << *text << '\n';
        else 
            std::cout << std::get<Json>(*result.output).dump(4) << '\n';
    }

    std::cout << "Steps: " << result.context.current_step << '\n';
    std::cout << "Events: " << result.context.events.size() << '\n';

    curl_global_cleanup();
    return 0;
}



