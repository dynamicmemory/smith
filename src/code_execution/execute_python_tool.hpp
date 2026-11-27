#pragma once 

#include "base_tool.hpp"

struct ExecutePythonTool : BaseTool {
    ExecutePythonTool();

    ToolResult execute(ExecutionContext &, const Json &arguments) override;

};
