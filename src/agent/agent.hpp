#pragma once 

#include "types.hpp"
#include "llm_client.hpp"
#include "in_memory_session_manager.hpp"
#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>

using AgentOutput = std::variant<std::string, Json>;

struct AgentResult {
    std::optional<AgentOutput> output;
    ExecutionContext context;
};

using BeforeToolCallback = std::function<std::optional<ToolResult>(
        ExecutionContext &, const ToolCall &)>;

using AfterToolCallback = std::function<std::optional<ToolResult>(
        ExecutionContext &, const ToolResult &)>;

using BeforeLlmCallback = std::function<std::optional<LlmResponse>(
        ExecutionContext &, LlmRequest &)>;

using OptimizationFunction = std::function<void(
        ExecutionContext &, 
        LlmRequest &)>;

struct Agent {
    Agent(LlmClient &model, 
          std::vector<BaseTool*> tools, 
          std::string instructions = "",
          int max_steps = 100, 
          std::string name = "agent",
          std::optional<Json> output_type = std::nullopt,
          std::vector<BeforeToolCallback> before_tool_callbacks = {},
          std::vector<AfterToolCallback> after_tool_callbacks = {},
          std::vector<BeforeLlmCallback> before_llm_callbacks = {},
          BaseSessionManager *session_manager = nullptr
   );

    AgentResult run(const std::string &user_input, 
                    const std::optional<std::string> &session_id = std::nullopt,
                    ExecutionContext *context = nullptr);

private:
    LlmClient &model;
    std::vector<BaseTool*> tools;
    std::unordered_map<std::string, BaseTool*> tools_by_name;
    std::string instructions;
    int max_steps;
    std::string name;
    std::optional<Json> output_type;
    std::optional<std::string> output_tool_name;
    std::unique_ptr<BaseTool> output_tool;
    std::vector<BeforeToolCallback> before_tool_callbacks;
    std::vector<AfterToolCallback> after_tool_callbacks;
    std::vector<BeforeLlmCallback> before_llm_callbacks;

    void setup_tools();
    void step(ExecutionContext &context);
    bool is_final_response(const Event &event);
    std::optional<AgentOutput> extract_final_result(const Event &event);
    LlmRequest prepare_llm_request(const ExecutionContext &context);
    LlmResponse think(const LlmRequest &request);
    std::vector<ToolResult> act(ExecutionContext &context, 
                                const std::vector<ToolCall> & tool_calls);
    std::unique_ptr<InMemorySessionManager> default_session_manager;
    BaseSessionManager *session_manager;
};
