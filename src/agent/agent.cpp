#include "agent.hpp"
#include "final_answer_tool.hpp"
#include <stdexcept>
#include <utility>

#include <iostream>
#include "optimizer_callback.hpp"

Agent::Agent(
    LlmClient &model, std::vector<BaseTool*> tools, std::string instructions, 
    int max_steps, std::string name, std::optional<Json> output_type,
    std::vector<BeforeToolCallback> before_tool_callbacks,
    std::vector<AfterToolCallback> after_tool_callbacks,
    std::vector<BeforeLlmCallback> before_llm_callbacks
    ) :
    model(model), tools(std::move(tools)), instructions(std::move(instructions)), 
    max_steps(max_steps), name(std::move(name)), 
    output_type(std::move(output_type)), 
    before_tool_callbacks(std::move(before_tool_callbacks)), 
    after_tool_callbacks(std::move(after_tool_callbacks)),
    before_llm_callbacks(std::move(before_llm_callbacks)) {
       
           setup_tools();
       }

void Agent::setup_tools() {
    if (output_type.has_value()) {
        output_tool = std::make_unique<FinalAnswerTool>(*output_type);
        tools.push_back(output_tool.get());
        output_tool_name = output_tool->get_name();
    }

    for (BaseTool *tool : tools) {
        if (tool == nullptr)
            throw std::invalid_argument("Agent received a null tool pointer");

        auto [it, inserted] = tools_by_name.emplace(tool->get_name(), tool);

        if (!inserted)
            throw std::invalid_argument("Duplicate tool name: " + tool->get_name());
    }
}

AgentResult Agent::run(const std::string &user_input, ExecutionContext *context) {
    ExecutionContext local_context;
    if (context == nullptr)
        context = &local_context;

    context->add_event(
                 make_event(
                     context->execution_id, 
                     "user", 
                     {Message{"user", user_input}}
                )
          );

    while (context->current_step < max_steps) {
        step(*context);

        if (!context->events.empty() && is_final_response(context->events.back())) {
            auto output = extract_final_result(context->events.back());
            if (std::holds_alternative<Json>(*output)) 
                context->final_result = std::get<Json>(*output);
            else 
                context->final_result = std::get<std::string>(*output);

            return AgentResult{output, *context};
        }
    }
    return AgentResult{std::nullopt, *context};
}

/* Checks if this even contains a final response.*/
bool Agent::is_final_response(const Event &event) {
    // structured output behaviour
    if (output_tool_name.has_value()) {
        for (const ContentItem &item : event.content) {
            const auto *result = std::get_if<ToolResult>(&item);

            if (result && result->name == output_tool_name.value() &&
                    result->status == ToolStatus::success)
                return true;
        }
        return false;
    }

    // Standard free text output behaviour
    bool has_assistant_message = false;

    for (const ContentItem &item : event.content) { 
        if (std::holds_alternative<ToolCall>(item) ||
            std::holds_alternative<ToolResult>(item)) 
            return false;
    
        if (const auto *message = std::get_if<Message>(&item)) 
            if (message->role == "assistant")
                has_assistant_message = true;
    }

    return has_assistant_message;
}

/* */
std::optional<AgentOutput> Agent::extract_final_result(const Event &event) {
    if (output_tool_name.has_value()) {
        for (const ContentItem &item : event.content) {
            const auto *result = std::get_if<ToolResult>(&item);

            if (result && result->name == output_tool_name.value() &&
                    result->status == ToolStatus::success &&
                    !result->content.empty()) 
                return AgentOutput{result->content[0]};
        }
        return std::nullopt;
    }

    for (const ContentItem &item : event.content) 
        if (const auto *message = std::get_if<Message>(&item))
            if (message->role == "assistant")
            return message->content;

    return std::nullopt;
}

LlmRequest Agent::prepare_llm_request(const ExecutionContext &context) {
    LlmRequest request;

    if (!instructions.empty())
        request.instructions.push_back(instructions);

    request.tools = tools;

    for (const Event &event : context.events) 
        for (const ContentItem &item : event.content)
            request.contents.push_back(item);

    if (output_tool_name.has_value())
        request.tool_choice = "required";
    else if (!tools.empty())
        request.tool_choice = "auto";

    return request;
}

LlmResponse Agent::think(const LlmRequest &request) {
    return model.generate(request);
}

std::vector<ToolResult> Agent::act(ExecutionContext &context, 
                                   const std::vector<ToolCall> &tool_calls) {

    std::vector<ToolResult> results;

    for (const ToolCall &call : tool_calls) {
        auto it = tools_by_name.find(call.name);

        if (it == tools_by_name.end()) {
            results.push_back({
                    call.tool_call_id,
                    call.name, 
                    ToolStatus::error,
                    {Json("Unknown tool: " + call.name)}
            });
            continue;
        }

        std::optional<ToolResult> tool_response;

        // Before tool callbacks 
        for (auto &callback : before_tool_callbacks) {
            tool_response = callback(context, call);

            if (tool_response.has_value())
                break;
        }

        if (!tool_response.has_value()) {

            try {
                tool_response = it->second->execute(context, call.arguments);
            }
            catch (const std::exception &e) {
                tool_response = ToolResult{ call.tool_call_id, call.name,
                                            ToolStatus::error, {Json(e.what())}
                };
            }
        }

        ToolResult tool_result = std::move(*tool_response);

        // Preserve the ID from the original LLM tool call
        tool_result.tool_call_id = call.tool_call_id;
        tool_result.name = call.name;

        // After tool callbacks
        for (auto &callback : after_tool_callbacks) {
            auto replacement = callback(context, tool_result);

            if (replacement.has_value()) {
                tool_result = std::move(*replacement);
                break;
            }
        }

        // again, preserve the ID from the original LLM tool call
        tool_result.tool_call_id = call.tool_call_id;
        tool_result.name = call.name;

        results.push_back(std::move(tool_result));
    }
    return results;
}

void Agent::step(ExecutionContext &context) {
    LlmRequest request = prepare_llm_request(context);
    std::optional<LlmResponse> response;

    // Before llm callback
    for (auto &callback : before_llm_callbacks) {
        response = callback(context, request);
        
        if (response.has_value())
            break;
    }

    // If no callback intercepted the request, call the llm
    if (!response.has_value()) {
        // std::cout << "Instructions: " << request.instructions.size()
        //   << " | Contents: " << request.contents.size()
          // << " | Tools: " << request.tools.size() << '\n';
        int estimated = estimate_tokens(request);

        response = think(request);

        int actual = response->usage_metadata.value("input_tokens", 0);
        std::cout << "Estimated: " << estimated << " | Actual: " << actual << '\n';
    }

    // Store token useage for the next optimization check 
    if (response->usage_metadata.contains("input_tokens"))
        context.state["input_tokens"] = response->usage_metadata.at("input_tokens");

    // TODO: We need to send the error the llm? 
    if (response->error_message.has_value())
        throw std::runtime_error("LLM error: " + response->error_message.value());

    context.add_event(make_event(context.execution_id, name, response->contents));

    std::vector<ToolCall> tool_calls;
    for (const ContentItem &item : response->contents) 
        if (const auto *call = std::get_if<ToolCall>(&item))
            tool_calls.push_back(*call);

    if (!tool_calls.empty()) {
        std::vector<ToolResult> results = act(context, tool_calls);
        context.add_event(
                make_event(
                    context.execution_id,
                    name,
                    std::vector<ContentItem>(
                        results.begin(),
                        results.end()
                    )
              )
        );
    }
    context.increment_step();
}

