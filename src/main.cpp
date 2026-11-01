#include "ollama.hpp"
#include "gaia.hpp"
#include <iostream> 
#include <vector>

double calculator(const std::string &operand, double first_number, double second_number);

double calculator(const std::string &operand, double a, double b) {
    if (operand == "Add") 
        return a + b;
    if (operand == "Subtract") 
        return a - b;
    if (operand == "Multiply") 
        return a * b;
    if (operand == "Divide") 
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
