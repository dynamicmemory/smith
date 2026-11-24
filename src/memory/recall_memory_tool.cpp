#include "recall_memory_tool.hpp"

#include <sstream>

RecallMemoryTool::RecallMemoryTool(MemoryManager &memory_manager)
    : BaseTool(
        "recall_memory_tool",
        "Search stored memories for relevant information from previous interactions.",
        {
            {"type", "function"},
            {"function", {
                {"name", "recall_memory_tool"},
                {"description", "Search stored memories for relevant information from previous interactions."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"query", {
                            {"type", "string"},
                            {"description", "The information or topic to search for."}
                        }}
                    }},
                    {"required", {"query"}}
                }}
            }}
        }
    ),
    memory_manager(memory_manager) {}

ToolResult RecallMemoryTool::execute(
    ExecutionContext &,
    const Json &arguments
) {
    const std::string query = arguments.at("query").get<std::string>();

    if (query.empty())
        return {"", "recall_memory_tool", ToolStatus::error,
                {Json("Memory search query cannot be empty")}};

    auto memories = memory_manager.search_memories(query, 3);

    if (memories.empty())
        return {"", "recall_memory_tool", ToolStatus::success,
                {Json("No relevant memories found.")}};

    return {"", "recall_memory_tool", ToolStatus::success,
            {Json(format_memories(memories))}};
}

std::string RecallMemoryTool::format_memories(
    const std::vector<Memory> &memories
) const {
    std::ostringstream output;

    for (size_t i = 0; i < memories.size(); ++i) {
        const Memory &memory = memories[i];

        output << "[Memory " << i + 1 << "]\n"
               << "- Summary: " << memory.summary << "\n"
               << "- Category: " << memory.category << "\n"
               << "- Content: " << memory.content << "\n";

        if (i + 1 < memories.size())
            output << '\n';
    }

    return output.str();
}
