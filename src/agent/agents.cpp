#include "agents.hpp"
#include "final_answer_tool.hpp"

Agent create_gaia_agent(LlmClient &client, std::vector<BaseTool*> tools) { 
    const std::string instructions = 
        "You are a general AI assistant. I will ask you a question. "
        "First, determine if you can solve this problem with your current capabilities and set "
        "“is_solvable” accordingly. "
        "If you can solve it, set “is_solvable” to true and provide your answer in “final_answer”. "
        "If you cannot solve it, set “is_solvable” to false and explain why in “unsolvable_reason”. "
        "Your final answer should be a number OR as few words as possible OR a comma- "
        "separated list of numbers and/or strings. "
        "If you are asked for a number, don’t use a comma to write your number; also don’t "
        "use units such as $ or a percent sign unless specified otherwise. "
        "If you are asked for a string, don’t use articles, neither abbreviations (e.g., for cities), "
        "and write the digits in plain text unless specified otherwise. "
        "If you are asked for a comma-separated list, apply the above rules depending on "
        "whether the element is a number or a string. "
        "Do not search for information you have already retrieved. "
        "Reuse previous search results. Only perform another search when "
        "the information you need is missing or insufficient from earlier results.";  

    const Json schema = {
        {"type", "object"},
        {"properties", {
            {"is_solvable", {{"type", "boolean"}}},
            {"unsolvable_reason", {{"type", "string"}}},
            {"final_answer", {{"type", "string"}}}
        }},
        {"required", {"is_solvable", "unsolvable_reason", "final_answer"}},
        {"additionalProperties", false}
    };

    return Agent(client, tools, instructions, 15, "gaia_agent", std::optional<Json>{schema});
}
