#pragma once 

#include "agent.hpp"

std::optional<ToolResult> approval_callback(ExecutionContext &, const ToolCall &call);

