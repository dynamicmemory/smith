#pragma once

#include "base_tool.hpp"
#include "task.hpp"

#include <vector>

struct CreateTasksTool : BaseTool {
    CreateTasksTool();

    ToolResult execute(ExecutionContext &context, const Json &arguments) override;

private:
    std::string format_tasks(const std::vector<Task> &tasks) const;
};
