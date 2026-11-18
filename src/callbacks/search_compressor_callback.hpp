#pragma once

#include "agent.hpp"

std::optional<ToolResult> search_compressor_callback(
        ExecutionContext &context, 
        const ToolResult &tool_result
);
