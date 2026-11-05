#include "ollama.hpp"
#include "gaia.hpp"
#include <iostream> 
#include <vector>

std::string model = "qwen3.5:9b";

double calculator_tool(const std::string &operand, double first_number, double second_number);
void run_calc_tool_test();


// tool function parameters
struct Parameters {
// Types 
// std::string "string", int "integer", float/double "number", bool "boolean",
// std::vector<T> "array", json object/dict "object"
    std::string name;
    std::string type;
    bool required;

    // Parameters features, can add more like min, max vals, etc
    std::vector<std::string> enum_values = {};
    std::string description = {};
};

// tool functions 
struct Tool {
    std::string name; 
    std::string description;
    std::vector<Parameters> parameters;
};

json function_to_input_schema(const Tool &tool);
json format_tool_definition(const std::string &name, const std::string &description, const json &params);
json function_to_tool_definition(const std::string &tool_name);
json tool_execution(const std::string &name, const json &args);
json simple_agent_loop(const std::string &sys_prompt, const std::string &question);

json function_to_input_schema(const Tool &tool) {
    json properties = json::object();
    json required = json::array();
    
    for (const Parameters &param : tool.parameters) {
        json definition = {
            {"type", param.type}
        };

        if (!param.description.empty())
            definition["description"] = param.description;

        if (!param.enum_values.empty())
            definition["enum"] = param.enum_values;

        properties[param.name] = definition;

        if (param.required)
            required.push_back(param.name);
    }

    return {
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

json format_tool_definition(const std::string &name, const std::string &description, const json &params) {
    return {
        {"type", "function"}, 
        {"function", {
            {"name", name},
            {"description", description},
            {"parameters", params}
        }}
    };
}

json function_to_tool_definition(const std::string &tool_name) {
    Tool tool;
    tool.name = tool_name;

    if (tool_name == "calculator_tool") {
        tool.description = "Performs basic arithmetic operations.";
        tool.parameters = {{"operator", "string", true,
                               {"add", "subtract", "multiply", "divide"},
                               "Arithmetic operation to perform."},
                           {"first_number", "number", true},
                           {"second_number", "number", true}
        };
    }
    else if (tool_name == "web_search_tool") {
        tool.description = "Search the internet for recent or extra information.";
        tool.parameters = {{"query", "string", true},
                           {"max_results", "integer", true}
        };
    }
    else { throw std::runtime_error("Unknown tool: " + tool_name); }

    json schema = function_to_input_schema(tool);
    return format_tool_definition(tool.name, tool.description, schema);
}

    
json tool_execution(const std::string &name, const json &args) {
    if (name == "calculator_tool") {
        double result = calculator_tool(args.at("operator").get<std::string>(), 
                                        args.at("first_number").get<double>(),
                                        args.at("second_number").get<double>()
                );
        return result;
    }
    else if (name == "web_search_tool") {
        auto results = web_search_tool(args.at("query").get<std::string>(),
                                  args.at("max_results").get<int>());

        if (std::holds_alternative<std::string>(results))
            return std::get<std::string>(results);

        json output = json::array();
        for (json const &res : std::get<std::vector<json>>(results))
            output.push_back(res); 
        return output;
    }

    throw std::runtime_error("Unknown tool: " + name);
}

json simple_agent_loop(const std::string &sys_prompt, const std::string &question) {
    std::vector<std::string> tools = {"calculator_tool", "web_search_tool"};    
    std::vector<json> tool_definitions; 

    for (const json tool_name : tools) {
        json tool = function_to_tool_definition(tool_name);
        tool_definitions.push_back(tool);
    }
    
    std::vector<Message> messages = {
        {"system", sys_prompt},
        {"user", question}
    };
    Options options = {.tools = tool_definitions};

    while (true) {
        json response = chat(model, messages, options);
        std::cout << "Response: " << response << '\n';

        const json &message = response["message"];
        json tool_calls = message.value("tool_calls", json::array());

        if (!tool_calls.empty()) {
            Message assistant {
                .role = "assistant",
                .content = message.value("content", ""),
                .tool_calls = tool_calls 
            };
            messages.push_back(assistant);

            for (const auto &call : response["message"].value("tool_calls", json::array())) {
                std::string name = call["function"]["name"];
                json args = call["function"]["arguments"];
                json tool_result = tool_execution(name, args);
                messages.push_back({"tool", tool_result.dump(), name});
            }
        }
        else { 
            return message.value("content", "");
        }
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    json response = simple_agent_loop("You are a good boy, use tools if you need too", 
                                      "What is the date today and what is the price of bitcoin today?");

    std::cout << response << '\n';

    // std::variant<std::vector<json>, std::string> results = search_web("Kipchoge's marathon world record", 2);
    //
    // for (json const &res : *results) 
    //     std::cout << "title: " << res["title"] << '\n'
            // << "url: " << res["url"] << '\n'
            // << "content: " << res["content"] << '\n' 
            // << "score: " << res["score"] << "\n\n";

    curl_global_cleanup();
    return 0;
}





// void run_gaia_benchmark() {
//     std::vector<std::string> models = {
//         "qwen3.5:9b",
//         // "qwen3.6:27b"
//     };
//
//     std::vector<GaiaProblem> problems = load_gaia_dataset("data/GAIA/level1.jsonl");
//     std::vector<GaiaProblem> subset(problems.begin(), problems.begin() + 20);
//     auto results = run_experiment(subset, models);
//     print_results(results);
// }


// void run_calc_tool_test() {
//     json calculator_tool = {
//         {"type", "function"},
//         {"function", {
//             {"name", "calculator"},
//             {"description", "Perform basic arithmetic operations."},
//             {"parameters", {
//                 {"type", "object"},
//                 {"properties", {
//                     {"operator", {
//                         {"type", "string"},
//                         {"enum", {"add", "subtract", "multiply", "divide"}}
//                     }},
//                     {"first_number", {{"type", "number"}}},
//                     {"second_number", {{"type", "number"}}},
//                 }},
//                 {"required", {"operator", "first_number", "second_number"}}
//             }}
//         }}
//     };
//
//     std::string q1 = "What is the capital city of korea?";
//     std::string q2 = "What is 1234 * 5678?";
//     json tools = json::array({calculator_tool});
//
//     std::vector<Message> msg = {{"user", q1, "none"}};
//     json res1 = normal_chat(model, msg, tools);
//     std::cout << q1 << '\n' << res1["message"]["content"] << "\nTool call: " << res1["message"]["tool_calls"] << "\n\n";
//     msg.push_back({"assistant", res1["message"]["content"], "none"});
//
//     msg.push_back({"user", q2, "none"});
//     json res2 = normal_chat(model, msg, tools);
//     std::cout << q2 << '\n' << res2["message"]["content"] << "\nTool call: " << res2["message"]["tool_calls"] << "\n\n";
//     msg.push_back({"assistant", res2["message"]["content"], "none"});
//
//     json tool_calls = res2["message"].value("tool_calls", json::array());
//     for (const auto &call : tool_calls ) {
//         std::string name = call["function"]["name"];
//         json args = call["function"]["arguments"];
//
//         if (name == "calculator") {
//             double result = calculator_tool(args["operator"], 
//                                        args["first_number"], 
//                                        args["second_number"]);
//             msg.push_back({"tool", std::to_string(result), name});
//         }
//
//     }
//     json res3 = normal_chat(model, msg, tools);
//     std::cout << q2 << '\n' << res3["message"]["content"] << "\nTool call: " << res3["message"]["tool_calls"] << "\n\n";
// }

double calculator_tool(const std::string &operand, double a, double b) {
    if (operand == "add") 
        return a + b;
    if (operand == "subtract") 
        return a - b;
    if (operand == "multiply") 
        return a * b;
    if (operand == "divide") 
        if (b == 0) throw std::runtime_error("Division by zero");
            return a / b;
    throw std::runtime_error("Invalid operator");
}
