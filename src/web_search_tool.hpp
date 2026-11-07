#pragma once 

#include "base_tool.hpp"

struct WebSearchTool : BaseTool {
    WebSearchTool();

    ToolResult execute(ExecutionContext &context, const Json &arguments) override;
};

