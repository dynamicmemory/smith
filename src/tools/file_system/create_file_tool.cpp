#include "create_file_tool.hpp"

#include <filesystem>
#include <fstream>
#include <string>

CreateFileTool::CreateFileTool() : BaseTool(
    "create_file_tool",
    "Create a new file and write content to it.",
    {
        {"type", "function"},
        {"function", {
            {"name", "create_file_tool"},
            {"description", "Create a new file and write content to it. "
                            "Will not overwrite existing files."},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {
                        {"type", "string"},
                        {"description", "Path of the file to create."}
                    }},
                    {"content", {
                        {"type", "string"},
                        {"description", "Content to write into the file."}
                    }}
                }},
                {"required", {"path", "content"}}
            }}
        }}
    }
) {}

ToolResult CreateFileTool::execute(ExecutionContext &, const Json &arguments) {
    namespace fs = std::filesystem;

    const std::string path = arguments.at("path").get<std::string>();
    const std::string content = arguments.at("content").get<std::string>();

    if (path.empty())
        return { "", "create_file_tool", ToolStatus::error,
                {Json("File path cannot be empty")} 
        }; 

    if (fs::exists(path))
        return { "", "create_file_tool", ToolStatus::error,
                {Json("File already exists: " + path)} 
        }; 
    
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open())
        return { "", "create_file_tool", ToolStatus::error,
                {Json("Could not create file: " + path)} 
        }; 

    file << content;
    file.close();

    if (file.fail())
        return { "", "create_file_tool", ToolStatus::error,
                {Json("Failed to write content to file: " + path)} 
        }; 

    return { "", "create_file_tool", ToolStatus::success,
                {Json("Successfully created file: " + path)} 
    };
}
