#pragma once 

#include "base_tool.hpp"

struct WebSearchTool : BaseTool {
    WebSearchTool();

    ToolResult execute(ExecutionContext &, const Json &arguments) override;
};

