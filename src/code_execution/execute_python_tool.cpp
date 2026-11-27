#include "execute_python_tool.hpp"
#include "sandbox.hpp"

ExecutePythonTool::ExecutePythonTool() : BaseTool(
   "execute_python_tool",
    "Execute Python code inside the isolated sandbox.",
    {
        {"type", "function"},
        {"function", {
            {"name", "execute_python_tool"},
            {"description", "Execute Python code and return stdout, stderr, and errors."},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"code", {
                        {"type", "string"},
                        {"description", "Python code to execute."}
                    }}
                }},
                {"required", {"code"}}
            }}
        }}
    }
) {}

ToolResult ExecutePythonTool::execute(ExecutionContext &context, const Json &arguments) {
    if (!context.code_env)
        return {"", "execute_python_tool", ToolStatus::error, 
            { Json("No sandbox is available.")}};

    try {
        SandboxResult result = context.code_env->execute(arguments.at("code").get<std::string>());

        return {"", "execute_python_tool", 
                result.success ? ToolStatus::success : ToolStatus::error,
               { Json{
                   {"stdout", result.stdout_output}, 
                   {"stderr", result.stderr_output},
                   {"error", result.error}
              }}
        };
    }
    catch (const std::exception &e) {
        return {"", "execute_python_tool", ToolStatus::error, {Json(e.what())}};
    }
}
