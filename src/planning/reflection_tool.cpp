#include "reflection_tool.hpp"

#include <stdexcept>

ReflectionTool::ReflectionTool()
    : BaseTool(
        "reflection_tool",
        R"(Pause and analyze progress before continuing.

WHEN TO USE:
1. PROGRESS REVIEW - After completing a meaningful step.
2. ERROR ANALYSIS - When a tool fails or returns unexpected results.
3. RESULT SYNTHESIS - When combining information from multiple sources.
4. SELF CHECK - Before providing the final answer.

WHEN NOT TO USE:
- After every single tool call.
- During simple, straightforward operations.
- When everything is proceeding as expected.

Provide an assessment of the current situation and the next direction.
Set need_replan to true if the current plan needs modification.)",
        {
            {"type", "function"},
            {"function", {
                {"name", "reflection_tool"},
                {"description", R"(Pause and analyze progress before continuing.

Use for progress reviews, tool error analysis, synthesizing multiple
results, and checking completeness before the final answer.

Do not use after every tool call or during straightforward operations.)"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"analysis", {
                            {"type", "string"},
                            {"description", "Assessment of the current situation and next direction"}
                        }},
                        {"need_replan", {
                            {"type", "boolean"},
                            {"description", "Whether the current plan needs modification"}
                        }}
                    }},
                    {"required", {"analysis"}},
                    {"additionalProperties", false}
                }}
            }}
        }
    ) {}

ToolResult ReflectionTool::execute(ExecutionContext &, const Json &arguments) {
    if (!arguments.contains("analysis") || !arguments["analysis"].is_string()) 
        throw std::invalid_argument("'analysis' must be a string");

    bool need_replan = false;

    if (arguments.contains("need_replan")) {
        if (!arguments["need_replan"].is_boolean())
            throw std::invalid_argument("'need_replan' must be a boolean");

        need_replan = arguments["need_replan"].get<bool>();
    }

    const std::string analysis = arguments["analysis"].get<std::string>();

    const std::string result = need_replan
        ? "Reflection recorded (REPLAN NEEDED): " + analysis 
        : "Reflection recorded: " + analysis;

    return { "", "reflection_tool", ToolStatus::success, {Json(result)} };
}
