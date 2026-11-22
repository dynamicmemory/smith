#include "optimizer_callback.hpp"

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <unordered_set>

#include <iostream> 

const std::unordered_map<std::string, std::string>
TOOLCALL_COMPACTION_RULES = {
    {"create_file_tool", "[Content saved to file]"}
};

const std::unordered_map<std::string, std::string>
TOOLRESULT_COMPACTION_RULES = {
    {"read_file_tool", "File content from {path}. Re-read if needed."},
    {"web_search_tool", "Search results processed. Query: {query}. Re-search if needed."}
};

BeforeLlmCallback create_optimizer_callback(
        OptimizationFunction apply_optimization, 
        int threshold) {

    return [apply_optimization, threshold](
            ExecutionContext &context, 
            LlmRequest &request) -> std::optional<LlmResponse> {
        
        int token_count = count_tokens(context);

        std::cout << "Token count: " << token_count << '\n';

        if (token_count < threshold) 
            return std::nullopt;

        apply_optimization(context, request);

        return std::nullopt;
    };
}

int count_tokens(const ExecutionContext &context) {
    if (!context.state.contains("input_tokens"))
        return 0;

    return context.state.at("input_tokens").get<int>();
}

static size_t count_bytes(const std::string &text) { return text.size(); }

int estimate_tokens(const LlmRequest &request) {
    size_t chars = 0;

    for (const std::string &instruction : request.instructions)
        chars += count_bytes(instruction);

    for (const ContentItem &item : request.contents) {
        std::visit([&](const auto &content) {
            using T = std::decay_t<decltype(content)>;

            if constexpr (std::is_same_v<T, Message>) 
                chars += content.content.size();
            else if constexpr (std::is_same_v<T, ToolCall>)
                chars += content.name.size() + content.arguments.dump().size();
            else if constexpr (std::is_same_v<T, ToolResult>) {
                chars += content.name.size(); 

                for (const Json &value : content.content)
                    chars += value.dump().size();
            }
        }, item);
    }

    for (const BaseTool *tool : request.tools)
        chars += tool->tool_definition().dump().size();

    if (request.tool_choice) 
        chars += request.tool_choice->size();

    return static_cast<int>(chars / 3);
}

void apply_sliding_window(ExecutionContext &, LlmRequest &request, int window) {
    auto &contents = request.contents;

    auto initial_prompt  = std::find_if(contents.begin(), contents.end(),
            [](const ContentItem &item) {
                const auto *message = std::get_if<Message>(&item);
                return message && message->role == "user";
                }
    );

    if (initial_prompt == contents.end())
        return ;

    auto preserved = std::next(initial_prompt);
    auto preserved_count = std::distance(preserved, contents.end());

    if (preserved_count > window) {
        auto keep_start = std::prev(contents.end(), window);
        contents.erase(preserved, keep_start);
    } 
}

void apply_compaction(ExecutionContext &context, LlmRequest &request) {

    std::unordered_map<std::string, Json> tool_call_args;

    for (const ContentItem &item : request.contents)
        if (const auto *call = std::get_if<ToolCall>(&item))
            tool_call_args[call->tool_call_id] = call->arguments;

    for (ContentItem &item : request.contents)
        if (auto *call = std::get_if<ToolCall>(&item)) {
            auto rule = TOOLCALL_COMPACTION_RULES.find(call->name);

            if (rule != TOOLCALL_COMPACTION_RULES.end())
                call->arguments["content"] = rule->second;
        }
        else if (auto *result = std::get_if<ToolResult>(&item)) {
            auto rule = TOOLRESULT_COMPACTION_RULES.find(result->name);

            if (rule == TOOLRESULT_COMPACTION_RULES.end())
                continue;

            auto args = tool_call_args.find(result->tool_call_id);
            if (args == tool_call_args.end())
                continue;

            std::string replacement = rule->second;

            for (auto it = args->second.begin(); it != args->second.end(); ++it) {
                const std::string placeholder = "{" + it.key() + "}";

                if (!it.value().is_string())
                    continue;

                const std::string value = it.value().get<std::string>();
                size_t position = replacement.find(placeholder);
                while (position != std::string::npos) {
                    replacement.replace(position, placeholder.size(), value);
                    position = replacement.find(placeholder, position + value.size());
                }
            }
            result->content = {Json(replacement)};
        }
    size_t index = 0;

    for (Event &event : context.events)
        for (ContentItem &item : event.content)
            item = request.contents.at(index++);
}

static std::string format_history_for_summary(
        const std::vector<ContentItem> &contents, auto start, auto end) {

    std::string history;

    for (auto it=start; it!=end; ++it) {
        std::visit([&](const auto &item) {
            using T = std::decay_t<decltype(item)>;

            // TODO: I would love to just build "isinstance" from python for this type of thing for C++
            if constexpr (std::is_same_v<T, Message>) 
                history += item.role + ": " + item.content + "\n";
            else if constexpr (std::is_same_v<T, ToolCall>)
                history += "Tool call (" + item.name + "): " + item.arguments.dump() + "\n";
            else if constexpr (std::is_same_v<T, ToolResult>) {
                history += "Tool result (" + item.name + "): ";

                for (const auto &content : item.content)
                    history += content.dump() + " ";

                history += "\n";
            }
        }, *it);
    }
    return history;
}

std::optional<std::string> generate_summary(const std::string &history, LlmClient &llm_client) {
    LlmRequest summary_request;

    // TODO Extract this system prompt for easier editing
    summary_request.instructions.push_back(
            "You are summarizing an AI agent's work progress. " 
            "summarize the conversation history by structuring your summary int:\n"
            "1. Key findings\n"
            "2. Tools used\n"
            "3. Current status\n"
            "Be concise. Focus on information that will help the agent continue its work "
            "Preserve important facts, decision, and outstanding tasks. Provide a structured summary.\n"
            "Execution History:\n"
    );

    summary_request.contents.push_back(Message{ "user", history });

    LlmResponse response = llm_client.generate(summary_request);

    // TODO: Not sure if im support to supply the llm with the error?
    if (response.error_message.has_value())
        return std::nullopt;

    for (const ContentItem &item : response.contents) 
        if (const auto *message = std::get_if<Message>(&item))
            return message->content;

    return std::nullopt;
}

void apply_summarization(ExecutionContext &context, 
                         LlmRequest &request, 
                         LlmClient &llm_client, 
                         int keep_recent) {

    auto &contents = request.contents;

    auto initial_prompt  = std::find_if(contents.begin(), contents.end(),
            [](const ContentItem &item) {
                const auto *message = std::get_if<Message>(&item);
                return message && message->role == "user";
                }
    );

    if (initial_prompt == contents.end())
        return ;

    auto summary_start = std::next(initial_prompt);
    auto recent_start = contents.end();

    if (std::distance(summary_start, contents.end()) <= keep_recent)
        return ;

    recent_start = std::prev(contents.end(), keep_recent);
    std::string history = format_history_for_summary(contents, summary_start, recent_start);

    auto summary = generate_summary(history, llm_client);
    if (!summary.has_value())
        return ;

    std::vector<ContentItem> optimized_contents;
    optimized_contents.push_back(*initial_prompt);
    optimized_contents.push_back(
            Message{ "system", "Previous conversation summary:\n" + *summary}
    );

    optimized_contents.insert(optimized_contents.end(), recent_start, contents.end());
    request.contents = std::move(optimized_contents);
}


