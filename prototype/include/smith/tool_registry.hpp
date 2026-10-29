#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

class ToolRegistry {
public:
    explicit ToolRegistry(std::filesystem::path project_root = std::filesystem::current_path());
    const nlohmann::json& definitions() const;
    std::string execute(const std::string& name, const nlohmann::json& arguments) const;

private:
    std::filesystem::path project_root_;
    nlohmann::json definitions_;
    std::filesystem::path resolve_path(const nlohmann::json& arguments) const;
};
