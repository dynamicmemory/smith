#include "smith/tool_registry.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

ToolRegistry::ToolRegistry(std::filesystem::path project_root)
    : project_root_(std::filesystem::weakly_canonical(std::move(project_root))) {
    definitions_ = nlohmann::json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "list_directory"},
                {"description", "List files and directories at a path relative to the project root. Use '.' for the project root."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {{"path", {{"type", "string"}, {"description", "Relative directory path; defaults to '.'"}}}}},
                    {"required", nlohmann::json::array()},
                    {"additionalProperties", false}
                }}
            }}
        },
        {
            {"type", "function"},
            {"function", {
                {"name", "read_file"},
                {"description", "Read a UTF-8 text file inside the project root. Do not use this for binary files."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {{"path", {{"type", "string"}, {"description", "Relative path to a text file inside the project"}}}}},
                    {"required", nlohmann::json::array({"path"})},
                    {"additionalProperties", false}
                }}
            }}
        },
        {
            {"type", "function"},
            {"function", {
                {"name", "read_files"},
                {"description", "Read several independent UTF-8 text files in one tool call. Prefer this over separate read_file calls when inspecting multiple files."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"paths", {
                            {"type", "array"},
                            {"items", {{"type", "string"}}},
                            {"minItems", 1},
                            {"maxItems", 32},
                            {"description", "Relative file paths to read, all inside the project root"}
                        }}
                    }},
                    {"required", nlohmann::json::array({"paths"})},
                    {"additionalProperties", false}
                }}
            }}
        }
    });
}

const nlohmann::json& ToolRegistry::definitions() const { return definitions_; }

std::filesystem::path ToolRegistry::resolve_path(const nlohmann::json& arguments) const {
    std::string path = ".";
    if (arguments.contains("path")) {
        if (!arguments["path"].is_string()) throw std::runtime_error("Tool argument 'path' must be a string");
        path = arguments["path"].get<std::string>();
    }
    const std::filesystem::path requested(path);
    if (requested.is_absolute()) throw std::runtime_error("Absolute paths are not allowed");
    const auto resolved = std::filesystem::weakly_canonical(project_root_ / requested);
    const auto relative = resolved.lexically_relative(project_root_);
    if (relative.empty() && resolved != project_root_) throw std::runtime_error("Path is outside the project root");
    if (relative.is_absolute() || (!relative.empty() && *relative.begin() == "..")) {
        throw std::runtime_error("Path is outside the project root");
    }
    return resolved;
}

std::string ToolRegistry::execute(const std::string& name, const nlohmann::json& arguments) const {
    try {
        if (!arguments.is_object()) throw std::runtime_error("Tool arguments must be an object");

        if (name == "list_directory") {
            const auto path = resolve_path(arguments);
            if (!std::filesystem::is_directory(path)) throw std::runtime_error("Path is not a directory");
            std::ostringstream output;
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                output << (entry.is_directory() ? "[dir]  " : "[file] ") << entry.path().filename().string() << '\n';
            }
            return output.str().empty() ? "(empty directory)" : output.str();
        }

        if (name == "read_file") {
            if (!arguments.contains("path")) throw std::runtime_error("Missing required argument: path");
            const auto path = resolve_path(arguments);
            if (!std::filesystem::is_regular_file(path)) throw std::runtime_error("Path is not a regular file");
            std::ifstream file(path, std::ios::binary);
            if (!file) throw std::runtime_error("Could not open file");
            std::ostringstream contents;
            contents << file.rdbuf();
            constexpr std::size_t max_bytes = 256 * 1024;
            std::string result = contents.str();
            if (result.size() > max_bytes) result = result.substr(0, max_bytes) + "\n[truncated at 256 KiB]";
            return result;
        }

        if (name == "read_files") {
            if (!arguments.contains("paths") || !arguments["paths"].is_array()) {
                throw std::runtime_error("Tool argument 'paths' must be an array");
            }
            const auto& paths = arguments["paths"];
            if (paths.empty()) throw std::runtime_error("At least one file path is required");
            if (paths.size() > 32) throw std::runtime_error("A maximum of 32 files can be read per call");

            constexpr std::size_t max_total_bytes = 512 * 1024;
            std::size_t total_bytes = 0;
            std::unordered_set<std::string> seen;
            std::ostringstream output;

            for (const auto& item : paths) {
                if (!item.is_string()) throw std::runtime_error("Every path must be a string");
                const std::string requested = item.get<std::string>();
                if (!seen.insert(requested).second) continue;

                output << "\n===== " << requested << " =====\n";
                const auto path = resolve_path(nlohmann::json{{"path", requested}});
                if (!std::filesystem::is_regular_file(path)) {
                    output << "[Tool error: Path is not a regular file]\n";
                    continue;
                }

                std::ifstream file(path, std::ios::binary);
                if (!file) {
                    output << "[Tool error: Could not open file]\n";
                    continue;
                }

                std::ostringstream contents;
                contents << file.rdbuf();
                std::string content = contents.str();
                if (content.size() > max_total_bytes - total_bytes) {
                    const std::size_t remaining = max_total_bytes - total_bytes;
                    content.resize(remaining);
                    output << content << "\n[batch output truncated at 512 KiB]\n";
                    break;
                }

                total_bytes += content.size();
                output << content << '\n';
            }
            return output.str();
        }

        throw std::runtime_error("Unknown tool: " + name);
    } catch (const std::exception& error) {
        return std::string("Tool error: ") + error.what();
    }
}
