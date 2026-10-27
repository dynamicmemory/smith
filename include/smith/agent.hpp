#pragma once

#include "smith/model_client.hpp"
#include "smith/tool_registry.hpp"

#include <cstddef>
#include <string>
#include <vector>

class Agent {
public:
    explicit Agent(ModelClient model, std::string system_prompt);
    std::string respond(const std::string& input, ModelClient::StreamCallback on_chunk);

private:
    ModelClient model_;
    ToolRegistry tools_;
    std::vector<ChatMessage> history_;
    static constexpr std::size_t max_tool_rounds_ = 16;
};
