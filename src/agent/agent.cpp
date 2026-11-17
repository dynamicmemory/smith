#include "agent.hpp"
#include "final_answer_tool.hpp"
#include <stdexcept>
#include <utility>

Agent::Agent(LlmClient &model, 
             std::vector<BaseTool*> tools, 
             std::string instructions, 
             int max_steps, 
             std::string name,
             std::optional<Json> output_type) :
       model(model), tools(std::move(tools)), instructions(std::move(instructions)), 
       max_steps(max_steps), name(std::move(name)), output_type(std::move(output_type)) {
       
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

        try {
            ToolResult result = it->second->execute(context, call.arguments);

            //Preserver the ID from the original LLM tool call, might be a problem 
            // as we are using tool name i think as the id.... cant remember TODO:
            result.tool_call_id = call.tool_call_id;
            result.name = call.name;

            results.push_back(std::move(result));
        }
        catch (const std::exception &e) {
             results.push_back({
                     call.tool_call_id,
                     call.name,
                     ToolStatus::error, 
                     {Json(e.what())}
            });
        }
    }
    return results;
}

void Agent::step(ExecutionContext &context) {
    LlmRequest request = prepare_llm_request(context);
    LlmResponse response = think(request);

    // TODO: We need to send the error the llm? 
    if (response.error_message.has_value())
        throw std::runtime_error("LLM error: " + response.error_message.value());

    context.add_event(make_event(context.execution_id, name, response.contents));

    std::vector<ToolCall> tool_calls;
    for (const ContentItem &item : response.contents) 
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

