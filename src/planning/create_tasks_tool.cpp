#include "create_tasks_tool.hpp"

#include <stdexcept>

CreateTasksTool::CreateTasksTool() : BaseTool(
        "create_tasks_tool",
        "Creates a task list to break down a problem that "
        "requires multiple thinking steps",
        {
            {"type", "function"},
            {"function", {
                {"name", "create_tasks_tool"},
                {"description", R"(Create or update a task plan.
                    WHEN TO USE:
                    - Complex queries requiring multiple research steps.
                    - Questions requiring information from multiple sources.
                    
                    WHEN NOT TO USE:
                    - Simple questions answerable with a single search.
                    - Tasks with obvious, straightforward procedures.
                    
                    HOW TO USE:
                    - Regenerate the entire task list when updating the plan.
                    - Mark completed tasks as "completed".
                    - Mark the next task to work on as "in_progress".
                    - Keep future tasks as "pending".
                    - Group related lookups into meaningful tasks.
                    - Consider what information is needed, what order is efficient,
                      and how completion of each task will be determined.)"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"tasks", {
                            {"type", "array"},
                            {"items", {
                                {"type", "object"},
                                {"properties", {
                                    {"content", {{"type", "string"}}},
                                    {"status", {
                                        {"type", "string"},
                                        {"enum", {"pending", "in_progress", "completed"}}
                                    }}
                                }},
                                {"required", {"content", "status"}},
                                {"additionalProperties", false}
                            }}
                        }}
                    }},
                    {"required", {"tasks"}},
                    {"additionalProperties", false}
                }}
            }}
        }
    ) {}

ToolResult CreateTasksTool::execute(ExecutionContext &, const Json &arguments) {
    if (!arguments.contains("tasks") || !arguments["tasks"].is_array())
        throw std::invalid_argument("'tasks' must be an array");

    std::vector<Task> tasks;

    for (const auto &item : arguments["tasks"]) {
        if (!item.is_object() || !item.contains("content") || !item["content"].is_string() || !item.contains("status") || !item["status"].is_string()) { 
            throw std::invalid_argument("Invalid task structure");
        }
     
        Task task;
        task.content = item["content"].get<std::string>();

        const std::string status = item["status"].get<std::string>();

        if (status == "pending")
            task.status = TaskStatus::pending;
        else if (status == "in_progress")
            task.status = TaskStatus::in_progress;
        else if (status == "completed")
            task.status = TaskStatus::completed;
        else 
            throw std::invalid_argument("Invalid task status: " + status);

        tasks.push_back(std::move(task));
    }

    return {"", "create_tasks_tool", ToolStatus::success, {Json(format_tasks(tasks))}};
}

std::string CreateTasksTool::format_tasks(const std::vector<Task> &tasks) const {
    std::string result;

    for (const auto &task : tasks) {
        switch (task.status) {
            case TaskStatus::pending:
                result += "[ ] " + task.content;
                break;

            case TaskStatus::in_progress:
                result += "[>] **" + task.content + "**";
                break;

            case TaskStatus::completed:
                result += "[x] ~~" + task.content + "~~";
                break;
        }

        result += "\n";
    }
    return result;
}


    
