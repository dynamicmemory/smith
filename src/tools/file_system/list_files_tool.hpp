#pragma once 

#include "base_tool.hpp"

struct ListFilesTool : BaseTool {
    ListFilesTool();

    ToolResult execute(ExecutionContext &, const Json &arguments) override;
};

