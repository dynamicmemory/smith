#include "read_file_tool.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

//TODO: Feel free to add more as I go
/* Supported text file extensions */
static const std::vector<std::string> TEXT_EXTENSIONS = { 
    ".txt", ".py", ".js", ".json", ".md", ".html",
    ".css", ".xml", ".yaml", ".yml", ".log", ".sh",
    ".cpp", ".c"
};

ReadFileTool::ReadFileTool() : BaseTool(
    "read_file_tool",
    "Read the contents of a text or CSV file.",
    {
        {"type", "function"},
        {"function", {
            {"name", "read_file_tool"},
            {"description", "Read the contents of a text or CSV file."
                            // "Read the contents of a text or CSV file. By default" 
                            // ", reads the ENTIRE file. Only specify start_line and "
                            // "end_line when you explicity need a certain range. "
                            // "Supports optional line ranges."},
                                                        },
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"path", {
                        {"type", "string"},
                        {"description", "Path to the file to read."}
                    }},
                    {"start_line", {
                        {"type", "integer"},
                        {"description", "First line to start reading from. Defaults to 1."}
                    }},
                    {"end_line", {
                        {"type", "integer"},
                        {"description", "Last line to read. Defaults to -1 (EOF)."},
                        // {"description", "OPTIONAL Last line to read too. Omit " 
                        //                 "to read until EOF. Defaults to -1 (EOF) "
                                        // "when no argument is provided."}
                    }}
                }},
                {"required", {"path"}}
            }}
        }}
    }
) {}

#include <iostream>
// TODO: Not implemented currently
// std::string read_exel_file(const std::string &path);

/* Reads a text file, optionally selecting a line range */
ToolResult read_text_file(const std::string &path, int start_line, int end_line) {
    std::ifstream file(path);

    if (!file.is_open())
        return { "", "read_file_tool", ToolStatus::error, 
                 {Json("Could not open file: " + path)} };

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line))
        lines.push_back(line);



    if (start_line < 1 || (end_line != -1 && end_line < start_line)) 
        return { "", "read_file_tool", ToolStatus::error, 
                 {Json("Start and end line indexes are incorrect")} };

    size_t start_idx = std::min(static_cast<size_t>(start_line - 1), lines.size());
    size_t end_idx = end_line == -1 ? lines.size() : 
                             std::min(static_cast<size_t>(end_line), lines.size());

    std::vector<std::string> selected_lines(lines.begin() + start_idx, lines.begin() + end_idx);

    std::cout << "LINES READ: " << selected_lines.size() << '\n';

    std::string result;
    size_t counter = start_line;
    for (size_t i=start_idx; i<end_idx; ++i)
        result += std::to_string(counter++) + " | " + lines[i] + '\n';

    return { "", "read_file_tool", ToolStatus::success, {Json(result)} };
}


/* Reads a csv file, vibed with gpt, checked by myself*/
ToolResult read_csv_file(const std::string &path) {
    std::ifstream file(path);

    if (!file.is_open())
        return { "", "read_file_tool", ToolStatus::error, 
                {Json("Could not open CSV file: " + path)} };

    std::string line;
    std::string result; 

    while (std::getline(file, line)) {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;

        for (size_t i=0; i<line.size(); ++i) {
            char c = line[i];
            if (c == ',' && !in_quotes) {
                fields.push_back(field);
                field.clear();
            }
            else if (c == '"') {
                if (in_quotes && i+1 < line.size() && line[i+1] == '"') {
                    field += '"';
                    ++i;
                }
                else {
                    in_quotes = !in_quotes;
                }
            }
            else {
                field += c;
            }
        }

        fields.push_back(field);

        for (size_t i=0; i<fields.size(); ++i) {
            result += fields[i];
            
            if (i+1 < fields.size())
                result += " | ";
        }
        result += '\n';
    }

    return { "", "read_file_tool", ToolStatus::success, {Json(result)} };
}


/* Execute read_file_tool call*/
ToolResult ReadFileTool::execute(ExecutionContext &, const Json &arguments) {
    namespace fs = std::filesystem;

    const std::string path = arguments.at("path").get<std::string>();
    const int start_line = arguments.value("start_line", 1);
    const int end_line = arguments.value("end_line", -1);

    std::cout << "PATH: " << path 
              << "\nSTART: " << start_line 
              << "\nEND: " << end_line << '\n';
    if (!fs::exists(path))
        return { "", "read_file_tool", ToolStatus::error, 
                {Json("File not found " + path)} };

    std::string extension = fs::path(path).extension().string();

    std::transform(extension.begin(), extension.end(), extension.begin(), 
                   [](unsigned char c) { return std::tolower(c); }
    );

    if (std::find(TEXT_EXTENSIONS.begin(), TEXT_EXTENSIONS.end(), extension) != TEXT_EXTENSIONS.end())
        return read_text_file(path, start_line, end_line);
    else if (extension == ".csv")
        return read_csv_file(path);
    // TODO: Excel files
    // else if (extension == ".xlsx" || exention == ".xls")
        // return read_excel_file(path);
    else 
        return { "", "read_file_tool", ToolStatus::error, 
                {Json("Unsupported file type, leave a pr for: " + extension)} };
}

