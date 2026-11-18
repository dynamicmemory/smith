#pragma once 

#include "base_tool.hpp"

struct DeleteFileTool : BaseTool {
    DeleteFileTool();
    
    ToolResult execute(ExecutionContext &, const Json &arguments) override;
};
