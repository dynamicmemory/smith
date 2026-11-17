#include "list_files_tool.hpp"

#include <filesystem>
#include <algorithm>
#include <string>
#include <vector>


ListFilesTool::ListFilesTool() : BaseTool(
        "list_files_tool",
        "List files and directories in a given path.",
        {
            {"type", "function"},
            {"function", {
                {"name", "list_files_tool"},
                {"description", "List files and directories in a given path."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"path", {
                            {"type", "string"},
                            {"description", "The directory path to list."}
                        }}
                    }},
                    {"required", {"path"}}
                }}
            }}
        }
    ){}


/* List files and directories in the given path*/
ToolResult ListFilesTool::execute(ExecutionContext &, const Json &arguments) { 
    namespace fs = std::filesystem;

    const std::string path = arguments.at("path").get<std::string>();
    fs::path directory(path); 

    if (!fs::exists(directory))
        return {"", "list_files_tool", ToolStatus::error, {Json("Path not found: " + path)}
        };

    if (!fs::is_directory(directory))
        return {"", "list_files_tool", ToolStatus::error, {Json("Not a directory: " + path)}
        };

    std::vector<std::string> dirs;
    std::vector<std::string> files;
    
    for (const auto &item : fs::directory_iterator(directory)) {
        std::string filename = item.path().filename().string();

        // TODO: We are skipping hidden files for future refference
        if (filename.starts_with('.'))
            continue;

        if (item.is_directory())
            dirs.push_back(filename+"/");
        else 
            files.push_back(filename);
    }

    std::sort(dirs.begin(), dirs.end(), [](const auto &a, const auto &b) {
        return a < b; });
    std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) {
        return a < b; });

    std::string result = "Directory: " + path + "\n";
    for (const auto &item : dirs) 
        result += item + '\n';
    for (const auto &item : files) 
        result += item + '\n';

    return { "", "list_files_tool", ToolStatus::success, {Json(result)} };
}

