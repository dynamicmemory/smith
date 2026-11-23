#pragma once 

#include <string>
#include <vector>
#include <variant>
#include <chrono> 
#include <nlohmann/json.hpp>
#include <utility>
#include <optional>

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

inline Event make_event(const std::string &execution_id, 
                        const std::string &author, 
                        std::vector<ContentItem> content) {

    return Event{"", execution_id, std::chrono::system_clock::now(), author, std::move(content)};
}

struct PendingToolCall {
    ToolCall tool_call;
    std::string confirmation_message;
};

struct ToolConfirmation { 
    std::string tool_call_id;
    std::optional<std::string> reason;
    bool approved;
    std::optional<Json> modified_arguments;
};

// ToolCall serialization
inline void to_json(Json &j, const ToolCall &t) {
    j = {
        {"tool_call_id", t.tool_call_id},
        {"name", t.name},
        {"arguments", t.arguments}
    };
}

inline void from_json(const Json &j, ToolCall &t) {
    j.at("tool_call_id").get_to(t.tool_call_id);
    j.at("name").get_to(t.name);
    j.at("arguments").get_to(t.arguments);
}

// PendingToolCall serialization
inline void to_json(Json &j, const PendingToolCall &p) {
    j = {
        {"tool_call", p.tool_call},
        {"confirmation_message", p.confirmation_message}
    };
}

inline void from_json(const Json &j, PendingToolCall &p) {
    j.at("tool_call").get_to(p.tool_call);
    j.at("confirmation_message").get_to(p.confirmation_message);
}

// ToolConfirmation serialization
inline void to_json(Json &j, const ToolConfirmation &c) {
    j = {
        {"tool_call_id", c.tool_call_id},
        {"reason", c.reason},
        {"approved", c.approved},
        {"modified_arguments", c.modified_arguments}
    };
}

inline void from_json(const Json &j, ToolConfirmation &c) {
    j.at("tool_call_id").get_to(c.tool_call_id);
    j.at("reason").get_to(c.reason);
    j.at("approved").get_to(c.approved);
    j.at("modified_arguments").get_to(c.modified_arguments);
}
