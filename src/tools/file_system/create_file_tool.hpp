#pragma once 

#include "base_tool.hpp"

struct CreateFileTool : BaseTool { 
    CreateFileTool();

    ToolResult execute(ExecutionContext &, const Json &arguments) override;
};
