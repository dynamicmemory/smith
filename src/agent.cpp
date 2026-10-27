#include "smith/agent.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

std::string lowercase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

// Some local models emit a plan as ordinary text instead of issuing the tool
// call they just described. Treat obvious progress narration as incomplete,
// but leave ordinary answers alone.
bool looks_like_progress_narration(const std::string& content) {
    const std::string text = lowercase(content);
    static const char* phrases[] = {
        "let me read",
        "let me inspect",
        "let me examine",
        "let me look at",
        "let me explore",
        "now let me",
        "now i'll",
        "now i will",
        "next i'll",
        "next i will",
        "i'll start by",
        "i will start by",
        "i'm going to inspect",
        "i am going to inspect",
        "i'll now",
        "i will now",
        "i'll check",
        "i will check",
        "i'll explore",
        "i will explore",
        "let's inspect",
        "let's read",
        "let's look at",
        "i have a complete picture",
        "now i have a complete picture"
    };

    for (const char* phrase : phrases) {
        if (text.find(phrase) != std::string::npos) return true;
    }
    return false;
}

std::string inspected_file_summary(const std::unordered_set<std::string>& files) {
    if (files.empty()) return "No files have been read yet.";
    std::string result = "Files already read this turn:";
    for (const auto& file : files) result += "\n- " + file;
    return result;
}

} // namespace

Agent::Agent(ModelClient model, std::string system_prompt)
    : model_(std::move(model)), tools_() {
    if (system_prompt.empty()) throw std::invalid_argument("System prompt cannot be empty");
    history_.push_back({"system", std::move(system_prompt)});
}

std::string Agent::respond(
    const std::string& input,
    ModelClient::StreamCallback on_chunk
) {
    if (input.empty()) {
        throw std::invalid_argument("User message cannot be empty");
    }

    const std::size_t previous_history_size = history_.size();
    history_.push_back({"user", input});

    std::unordered_map<std::string, std::string> executed_tools;
    std::unordered_map<std::string, std::size_t> repeated_calls;
    std::unordered_set<std::string> inspected_files;
    bool used_tools_this_turn = false;
    std::size_t continuation_attempts = 0;
    constexpr std::size_t max_continuation_attempts = 3;

    auto finish_with_notice = [&](const std::string& notice) {
        history_.push_back({"assistant", notice});
        if (on_chunk) on_chunk(notice);
        return notice;
    };

    try {
        for (std::size_t round = 0; round < max_tool_rounds_; ++round) {
            // Buffer each model round. Intermediate narration accompanying tool
            // calls should not be shown as if it were the final answer.
            ModelResponse response = model_.chat(
                history_,
                tools_.definitions(),
                [](const std::string&) {}
            );

            if (response.tool_calls.empty()) {
                if (used_tools_this_turn &&
                    continuation_attempts < max_continuation_attempts &&
                    looks_like_progress_narration(response.content)) {
                    history_.push_back({"assistant", response.content});
                    history_.push_back({
                        "user",
                        "INTERNAL CONTINUATION: You have not completed the user's original task. "
                        "Do not narrate plans or repeat progress updates. Continue the original task "
                        "now: use the available tools to inspect anything still needed, then provide "
                        "the final answer. Do not repeat files already read unless necessary.\n" +
                        inspected_file_summary(inspected_files)
                    });
                    ++continuation_attempts;
                    continue;
                }

                history_.push_back({"assistant", response.content});
                if (on_chunk && !response.content.empty()) on_chunk(response.content);
                return response.content;
            }

            used_tools_this_turn = true;
            nlohmann::json calls = nlohmann::json::array();
            for (const auto& call : response.tool_calls) {
                calls.push_back({
                    {"type", "function"},
                    {"function", {
                        {"name", call.name},
                        {"arguments", call.arguments}
                    }}
                });
            }
            history_.push_back({"assistant", response.content, calls});
            bool repeated_call_limit_reached = false;

            for (const auto& call : response.tool_calls) {
                if (call.name == "read_file" &&
                    call.arguments.is_object() &&
                    call.arguments.contains("path") &&
                    call.arguments["path"].is_string()) {
                    inspected_files.insert(call.arguments["path"].get<std::string>());
                }
                if (call.name == "read_files" &&
                    call.arguments.is_object() &&
                    call.arguments.contains("paths") &&
                    call.arguments["paths"].is_array()) {
                    for (const auto& path : call.arguments["paths"]) {
                        if (path.is_string()) inspected_files.insert(path.get<std::string>());
                    }
                }

                const std::string key = call.name + "\n" + call.arguments.dump();
                auto cached = executed_tools.find(key);

                if (cached != executed_tools.end()) {
                    const std::size_t repeats = ++repeated_calls[key];
                    if (repeats >= 2) {
                        repeated_call_limit_reached = true;
                        history_.push_back({
                            "tool",
                            "Repeated tool call blocked: this exact call has already been "
                            "requested multiple times. Do not repeat it. Previous result:\n" + cached->second,
                            nullptr,
                            call.name
                        });
                        continue;
                    }

                    history_.push_back({
                        "tool",
                        "This exact tool call has already been executed during this turn. "
                        "Do not repeat it; use the previous result and continue. Previous result:\n" + cached->second,
                        nullptr,
                        call.name
                    });
                    continue;
                }

                const std::string result = tools_.execute(call.name, call.arguments);
                executed_tools.emplace(key, result);
                history_.push_back({"tool", result, nullptr, call.name});
            }

            if (repeated_call_limit_reached) {
                return finish_with_notice(
                    "I stopped because the model repeatedly requested the same tool "
                    "with identical arguments. Repeated calls were not executed. "
                    "Please narrow the task or try again."
                );
            }
        }

        return finish_with_notice(
            "I reached the tool-use limit before completing the task. I stopped rather "
            "than continuing to make tool calls without finishing. Please ask me to "
            "continue from the files already inspected."
        );
    } catch (...) {
        history_.resize(previous_history_size);
        throw;
    }
}
