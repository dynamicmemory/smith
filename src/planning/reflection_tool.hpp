#pragma once 

#include "base_tool.hpp"

struct ReflectionTool : BaseTool {
    ReflectionTool();

    ToolResult execute(ExecutionContext &context, const Json &arguments) override;

private:
    std::string analysis;
    bool need_replan;
};
