#pragma once

#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct ChatMessage {
    std::string role;
    std::string content;
    nlohmann::json tool_calls = nullptr;
    std::string tool_name;
};

struct ToolCall {
    std::string name;
    nlohmann::json arguments = nlohmann::json::object();
};

struct ModelResponse {
    std::string content;
    std::vector<ToolCall> tool_calls;
};

class ModelClient {
public:
    using StreamCallback = std::function<void(const std::string&)>;

    explicit ModelClient(
        std::string model,
        std::string base_url = "http://127.0.0.1:11434"
    );

    ModelResponse chat(
        const std::vector<ChatMessage>& messages,
        const nlohmann::json& tools,
        StreamCallback on_chunk
    ) const;

private:
    std::string model_;
    std::string endpoint_;
};
