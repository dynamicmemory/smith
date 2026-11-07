#pragma once 

#include "execution_context.hpp"

/* Abstract base class for all tools */
struct BaseTool {
    BaseTool(std::string name, std::string description, Json definition) :
        name(std::move(name)),
        description(std::move(description)),
        definition(std::move(definition)) {}

    virtual ~BaseTool() = default;

    virtual ToolResult execute(ExecutionContext &context, 
                               const Json &arguments) = 0;
    
    const Json &tool_definition() const { return definition; }
    const std::string &get_name() const { return name; }
    const std::string &get_description() const { return description; }

private:
    std::string name;
    std::string description;
    Json definition;
};
