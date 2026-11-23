#pragma once 

#include "execution_context.hpp"

/* Abstract base class for all tools */
struct BaseTool {
    BaseTool(
        std::string name, 
        std::string description, 
        Json definition,
        bool requires_confirmation = false,
        std::string confirmation_message_template = 
            "The agent wants to execute '{name}' with arguments: {arguments}. Do you approve?"
    ) :
        name(std::move(name)),
        description(std::move(description)),
        definition(std::move(definition)),
        requires_confirmation(requires_confirmation), 
        confirmation_message_template(std::move(confirmation_message_template))
    {}

    virtual ~BaseTool() = default;

    virtual ToolResult execute(ExecutionContext &context, 
                               const Json &arguments) = 0;
    
    const Json &tool_definition() const { return definition; }
    const std::string &get_name() const { return name; }
    const std::string &get_description() const { return description; }

    bool needs_confirmation() const { return requires_confirmation; }

    std::string get_confirmation_message(const Json &arguments) const {
        std::string message = confirmation_message_template;

        auto replace = [&message](const std::string &placeholder, const std::string &value) {
            size_t pos = message.find(placeholder);
            if (pos != std::string::npos)
                message.replace(pos, placeholder.length(), value);
        };

        replace("{name}", name);
        replace("{arguments}", arguments.dump());

        for (auto it = arguments.begin(); it!=arguments.end(); ++it) 
            replace("{arguments[" + it.key() + "]}",
                    it.value().is_string() ? it.value().get<std::string>() : it.value().dump()
            );

        return message;
    }

private:
    std::string name;
    std::string description;
    Json definition;

    bool requires_confirmation;
    std::string confirmation_message_template;
};
