#pragma once 

#include <string>
#include <vector>
#include <variant>
#include <chrono> 
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

/* Text message in the conversation */
struct Message {
    std::string role;
    std::string content;
};

/* LLM's request to execute a tool. */
struct ToolCall {
    std::string tool_call_id;
    std::string name;
    Json arguments;
};

/* Possible return status's for tools */
enum class ToolStatus { success, error };

/* Result from tool execution */
struct ToolResult {
    std::string tool_call_id;
    std::string name;
    ToolStatus status;
    std::vector<Json> content;
};

/*  */
using ContentItem = std::variant<Message, ToolCall, ToolResult>;

/* Recorded occurrence during agent execution */
struct Event {
    std::string id;               // Needs uuid? 
    std::string execution_id;   
    std::chrono::system_clock::time_point timestamp;
    std::string author;
    std::vector<ContentItem> content;
};
