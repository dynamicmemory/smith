#pragma once 

#include "base_tool.hpp"

struct ReadFileTool : BaseTool {
    ReadFileTool();

    ToolResult execute(ExecutionContext &, const Json &arguments) override;
};

