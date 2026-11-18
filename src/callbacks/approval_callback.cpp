#include "approval_callback.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {
    const std::vector<std::string> DANGEROUS_TOOLS = {"delete_file_tool", 
                                                      "send_email_tool", 
                                                      "execute_sql_tool"
    };
}

/* Requests user approval before executing a "dangerous" tool. */
std::optional<ToolResult> approval_callback(ExecutionContext &, const ToolCall &call) {
    if ( std::find(DANGEROUS_TOOLS.begin(), 
                   DANGEROUS_TOOLS.end(), 
                   call.name) == DANGEROUS_TOOLS.end() )
        return std::nullopt;

    std::cout << "\nDangerous tool execution requested\n" 
              << "Tool: " << call.name << '\n'
              << "Arguments: " << call.arguments.dump(4) << '\n'
              << "Execute? (y/n): ";

    std::string response;
    std::getline(std::cin, response);

    if (response == "y" || response == "Y") {
        std::cout << "Approved. Executing... \n";
        return std::nullopt;
    }

    std::cout << "Denied. Skipping execution. \n";

    return ToolResult{ call.tool_call_id, call.name, ToolStatus::error, 
                       {Json("User denied execution of " + call.name)}
    };
}
