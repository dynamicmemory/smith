#pragma once

#include "types.hpp"
#include "base_tool.hpp"
#include <string>
#include <vector>
#include <optional>

/* Request object for LLM calls. */
struct LlmRequest {
    std::vector<std::string> instructions;
    std::vector<ContentItem> contents;
    std::vector<BaseTool*> tools;
    std::optional<std::string> tool_choice;
};

/* Response object from LLM calls. */
struct LlmResponse {
    std::vector<ContentItem> contents;
    std::optional<std::string> error_message;
    Json usage_metadata = Json::object();
};



