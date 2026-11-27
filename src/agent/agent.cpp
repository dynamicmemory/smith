#include "agent.hpp"
#include "final_answer_tool.hpp"
#include "execute_python_tool.hpp"
#include "sandbox.hpp"
#include <stdexcept>
#include <utility>

#include <iostream>
#include "optimizer_callback.hpp"

Agent::Agent(
    LlmClient &model, std::vector<BaseTool*> tools, std::string instructions, 
    int max_steps, std::string name, std::optional<Json> output_type,
    std::vector<BeforeToolCallback> before_tool_callbacks,
    std::vector<AfterToolCallback> after_tool_callbacks,
    std::vector<BeforeLlmCallback> before_llm_callbacks,
    std::vector<AfterRunCallback> after_run_callbacks,
    std::optional<std::string> code_execution,
    BaseSessionManager *session_manager
    ) :
    model(model), tools(std::move(tools)), instructions(std::move(instructions)), 
    max_steps(max_steps), name(std::move(name)), 
    output_type(std::move(output_type)), 
    before_tool_callbacks(std::move(before_tool_callbacks)), 
    after_tool_callbacks(std::move(after_tool_callbacks)),
    before_llm_callbacks(std::move(before_llm_callbacks)),
    after_run_callbacks(std::move(after_run_callbacks)),
    code_execution(std::move(code_execution)),
    default_session_manager( 
            session_manager ? nullptr : std::make_unique<InMemorySessionManager>()
    ),
    session_manager(
            session_manager ? session_manager : default_session_manager.get()
    )

    { setup_tools(); }

void Agent::setup_tools() {
    if (output_type.has_value()) {
        output_tool = std::make_unique<FinalAnswerTool>(*output_type);
        tools.push_back(output_tool.get());
        output_tool_name = output_tool->get_name();
    }

    // Handles sandbox and python execution tool 
    if (code_execution == "bubblewrap") {
        execute_python_tool = std::make_unique<ExecutePythonTool>();
        tools.push_back(execute_python_tool.get());
    }

    for (BaseTool *tool : tools) {
        if (tool == nullptr)
            throw std::invalid_argument("Agent received a null tool pointer");

        auto [it, inserted] = tools_by_name.emplace(tool->get_name(), tool);

        if (!inserted)
            throw std::invalid_argument("Duplicate tool name: " + tool->get_name());
    }
}

AgentResult Agent::run(
        const std::string &user_input, 
        const std::optional<std::string> &session_id,
        ExecutionContext *context,
        const std::vector<ToolConfirmation> &tool_confirmations) {

    std::shared_ptr<Session> session;

    // Loads session from provided id or else creates a new one
    if (session_id.has_value())
        session = session_manager->get_or_create(session_id.value());
    else 
        session = std::make_shared<Session>();

    ExecutionContext local_context;
    if (context == nullptr) {
        local_context.session = session;
        local_context.session_manager = session_manager;
        context = &local_context;
    }

    // Store restricted tool confirmations supplied by the user
    if (!tool_confirmations.empty())
        context->state()["tool_confirmations"] = tool_confirmations;

    // add users input if provided
    if (!user_input.empty())
        context->add_event(
            make_event(
                context->execution_id, 
                "user", 
                {Message{"user", user_input}}
            )
        );

    while (context->current_step < max_steps) {
        step(*context);

        // Return instantely if tools are awaiting confirmations
        if (context->state().contains("pending_tool_calls")) {
            auto pending_calls = context->state().at("pending_tool_calls").get<std::vector<PendingToolCall>>();
            session_manager->save(*session);

            return AgentResult{
                std::nullopt, 
                *context, 
                AgentStatus::pending_confirmation,
                pending_calls 
            };
        }
        
        // Check for final response 
        if (!context->events().empty() && is_final_response(context->events().back())) {
            auto output = extract_final_result(context->events().back());
            if (output.has_value()) {
                if (std::holds_alternative<Json>(*output)) 
                    context->final_result = std::get<Json>(*output);
                else 
                    context->final_result = std::get<std::string>(*output);
            }

            session_manager->save(*session);

            // After run callback for memory extraction and saving 
            for (auto &callback : after_run_callbacks)
                callback(*context);

            return AgentResult{output, *context, AgentStatus::complete, {}};
        }
    }

   session_manager->save(*session);
   
   for (auto &callback : after_run_callbacks)
       callback(*context);

    return AgentResult{
        std::nullopt,
        *context, 
        AgentStatus::complete,
        {}
    };
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


LlmRequest Agent::prepare_llm_request(ExecutionContext &context) {
    LlmRequest request;

    if (context.state().contains("pending_tool_calls") &&
            context.state().contains("tool_confirmations")) {
    
        auto results = process_confirmations(context);
        std::vector<ContentItem> content;

        for (auto &result : results)
            content.emplace_back(std::move(result));

        if (!content.empty())
            context.add_event(
                    make_event(context.execution_id, name, content)
            );

        context.state().erase("pending_tool_calls");
        context.state().erase("tool_confirmations");
    }

    if (!instructions.empty())
        request.instructions.push_back(instructions);

    request.tools = tools;

    for (const Event &event : context.events()) 
        for (const ContentItem &item : event.content)
            request.contents.push_back(item);

    if (output_tool_name.has_value())
        request.tool_choice = "required";
    else if (!tools.empty())
        request.tool_choice = "auto";

    return request;
}


std::vector<ToolResult> Agent::process_confirmations(ExecutionContext &context) {
    std::vector<ToolResult> results;

    // Resotre the pending calls from the session state 
    auto pending_calls = context.state().at("pending_tool_calls").get<std::vector<PendingToolCall>>();

    // Restore user decisions 
    auto confirmations = context.state().at("tool_confirmations").get<std::vector<ToolConfirmation>>();

    // Index confirmations by tool call id 
    std::unordered_map<std::string, ToolConfirmation> confirmation_map;

    for (const auto &confirmation : confirmations)
        confirmation_map[confirmation.tool_call_id] = confirmation;

    // Process every pending call. 
    for (const auto &pending : pending_calls) {
        const ToolCall &call = pending.tool_call;
        auto tool_it = tools_by_name.find(call.name);
        auto confirmation_it = confirmation_map.find(call.tool_call_id);

        if (tool_it == tools_by_name.end()) {
            results.push_back({
                    call.tool_call_id,
                    call.name,
                    ToolStatus::error,
                    {Json("Unknown tool: " + call.name)}
            });
            continue;
        }

        // Reject unless explicitly approved 
        if (confirmation_it == confirmation_map.end() || !confirmation_it->second.approved) {
            std::string reason = "Tool execution was not approved";

            if (confirmation_it != confirmation_map.end()) {
                const auto &user_reason = confirmation_it->second.reason;
                reason = user_reason.value_or("Tool execution was rejected by user.");
            }

            results.push_back({
                    call.tool_call_id,
                    call.name,
                    ToolStatus::error, 
                    {Json(reason)}
            });
            continue;
        }

        // Merge original arguments with user modifications
        Json arguments = call.arguments;
        const auto &modified = confirmation_it->second.modified_arguments;
        
        if (modified.has_value() && modified->is_object())
            for (auto it = modified->begin(); it!=modified->end(); ++it)
                arguments[it.key()] = it.value();

        BaseTool *tool = tool_it->second;

        ToolCall approved_call = call;
        approved_call.arguments = arguments;

        std::optional<ToolResult> tool_response;

        // Before tool callbacks 
        for (auto &callback : before_tool_callbacks) {
            tool_response = callback(context, approved_call);

            if (tool_response.has_value())
                break;
        }

        if (!tool_response.has_value()) 
            try {
                tool_response = tool->execute(context, arguments);
            }
            catch (const std::exception &e) {
                tool_response = ToolResult {
                    call.tool_call_id,
                    call.name,
                    ToolStatus::error,
                    {Json(e.what())}
                };
            }

        ToolResult tool_result = std::move(*tool_response);
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

        tool_result.tool_call_id = call.tool_call_id;
        tool_result.name = call.name;
        results.push_back(std::move(tool_result));
    }

    return results;
}

LlmResponse Agent::think(const LlmRequest &request) {
    return model.generate(request);
}

std::vector<ToolResult> Agent::act(ExecutionContext &context, 
                                   const std::vector<ToolCall> &tool_calls) {

    std::vector<ToolResult> results;
    std::vector<PendingToolCall> pending_calls;

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
        
        BaseTool *tool = it->second;

        if (tool->needs_confirmation()) {
            pending_calls.push_back({call, tool->get_confirmation_message(call.arguments)});

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

    if (!pending_calls.empty())
        context.state()["pending_tool_calls"] = pending_calls;

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

        std::cout << "\n[LLM] Sending request..." << std::endl;
        response = think(request);
        std::cout << "[LLM] Response received." << std::endl;

        int actual = response->usage_metadata.value("input_tokens", 0);
        std::cout << "Estimated: " << estimated << " | Actual: " << actual << '\n';
    }

    // Store token useage for the next optimization check 
    if (response->usage_metadata.contains("input_tokens"))
        context.state()["input_tokens"] = response->usage_metadata.at("input_tokens");

    // TODO: We need to send the error the llm? 
    if (response->error_message.has_value())
        throw std::runtime_error("LLM error: " + response->error_message.value());

    context.add_event(make_event(context.execution_id, name, response->contents));

    for (const auto &item : response->contents) {
        std::visit([](const auto &value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, Message>)
                std::cout << "[MESSAGE] " << value.content << '\n';
                else if constexpr (std::is_same_v<T, ToolCall>)
                std::cout << "[TOOL CALL] " << value.name
                << " " << value.arguments.dump(2) << '\n';
                }, item);
    }



    std::vector<ToolCall> tool_calls;
    for (const ContentItem &item : response->contents) 
        if (const auto *call = std::get_if<ToolCall>(&item))
            tool_calls.push_back(*call);

    if (!tool_calls.empty()) {
        std::vector<ToolResult> results = act(context, tool_calls);

        for (const auto &result : results) {
            std::cout << "[TOOL RESULT] " << result.name << '\n';

            for (const auto &content : result.content)
                std::cout << content.dump(2) << '\n';
        }

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

