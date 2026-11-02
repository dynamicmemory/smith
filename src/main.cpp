#include "ollama.hpp"
#include "gaia.hpp"
#include <iostream> 
#include <vector>

std::string model = "qwen3.5:9b";

double calculator(const std::string &operand, double first_number, double second_number);

double calculator(const std::string &operand, double a, double b) {
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

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    json calculator_tool = {
        {"type", "function"},
        {"function", {
            {"name", "calculator"},
            {"description", "Perform basic arithmetic operations."},
            {"parameters", {
                {"type", "object"},
                {"properties", {
                    {"operator", {
                        {"type", "string"},
                        {"enum", {"add", "subtract", "multiply", "divide"}}
                    }},
                    {"first_number", {{"type", "number"}}},
                    {"second_number", {{"type", "number"}}},
                }},
                {"required", {"operator", "first_number", "second_number"}}
            }}
        }}
    };

    std::string q1 = "What is the capital city of korea?";
    std::string q2 = "What is 1234 * 5678?";
    json tools = json::array({calculator_tool});

    std::vector<Message> msg = {{"user", q1, "none"}};
    json res1 = normal_chat(model, msg, tools);
    std::cout << q1 << '\n' << res1["message"]["content"] << "\nTool call: " << res1["message"]["tool_calls"] << "\n\n";
    msg.push_back({"assistant", res1["message"]["content"], "none"});

    msg.push_back({"user", q2, "none"});
    json res2 = normal_chat(model, msg, tools);
    std::cout << q2 << '\n' << res2["message"]["content"] << "\nTool call: " << res2["message"]["tool_calls"] << "\n\n";
    msg.push_back({"assistant", res2["message"]["content"], "none"});

    json tool_calls = res2["message"].value("tool_calls", json::array());
    for (const auto &call : tool_calls ) {
        std::string name = call["function"]["name"];
        json args = call["function"]["arguments"];

        if (name == "calculator") {
            double result = calculator(args["operator"], 
                                       args["first_number"], 
                                       args["second_number"]);
            msg.push_back({"tool", std::to_string(result), name});
        }

    }
    json res3 = normal_chat(model, msg, tools);
    std::cout << q2 << '\n' << res3["message"]["content"] << "\nTool call: " << res3["message"]["tool_calls"] << "\n\n";

    curl_global_cleanup();
    return 0;
}

void run_gaia_benchmark() {
    std::vector<std::string> models = {
        "qwen3.5:9b",
        // "qwen3.6:27b"
    };

    std::vector<GaiaProblem> problems = load_gaia_dataset("data/GAIA/level1.jsonl");
    std::vector<GaiaProblem> subset(problems.begin(), problems.begin() + 20);
    auto results = run_experiment(subset, models);
    print_results(results);
}
