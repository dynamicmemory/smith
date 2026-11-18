#include "delete_file_tool.hpp"

DeleteFileTool::DeleteFileTool() : BaseTool(
    "delete_file_tool",
        "Deletes a file. This action cannot be undone.",
        Json{
            {"type", "function"},
            {"function", {
                {"name", "delete_file_tool"},
                {"description", "Deletes a file. This action cannot be undone."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"file_path", {
                            {"type", "string"},
                            {"description", "Path of the file to delete"}
                        }}
                    }},
                    {"required", {"file_path"}}
                }}
            }}
        }
    ) {}

ToolResult DeleteFileTool::execute(ExecutionContext &, const Json &arguments) {
    const std::string path = arguments.at("file_path");

    return { "", "", ToolStatus::success, 
           { Json("File " + path + " would have been deleted")} 
    };
}
