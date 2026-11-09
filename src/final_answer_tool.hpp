#pragma once 

#include "base_tool.hpp"

struct FinalAnswerTool : BaseTool {
    explicit FinalAnswerTool(Json output_schema) : BaseTool(
            "final_answer", 
            "Return the final structured answer matching the required schema.",
            build_definition(output_schema)) {}

    ToolResult execute(ExecutionContext &context, const Json &arguments) override {
        return ToolResult{"", get_name(), ToolStatus::success, {arguments} };
    }

private: 
    static Json build_definition(const Json &output_schema) {
        return {
            {"type", "function"},
            {"function", {
                {"name", "final_answer"},
                {"description", "Return the final structured answer matching the required schema."},
                {"parameters", output_schema}
            }}
        };
    }
};
