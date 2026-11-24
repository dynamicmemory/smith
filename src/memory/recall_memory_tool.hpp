#pragma once 

#include "base_tool.hpp"
#include "memory_manager.hpp"

struct RecallMemoryTool : BaseTool {
    explicit RecallMemoryTool(MemoryManager &memory_manager);

    ToolResult execute(ExecutionContext &context, const Json &arguments) override;

private: 
    MemoryManager &memory_manager;
    std::string format_memories(const std::vector<Memory> &memories) const;
};
