#pragma once 
#include <string>
#include <vector>
#include <optional>
#include <map>
#include "agent.hpp"
#include "llm_client.hpp"
#include "web_search_tool.hpp"

// #include "ollama.hpp"

struct GaiaOutput {
    bool is_solvable;
    std::string unsolvable_reason;
    std::string final_answer;
};

struct GaiaProblem {
    std::string task_id;
    std::string question;
    std::string answer;
};

struct GaiaEval {
    std::string task_id;
    std::string model;
    bool correct;
    std::optional<bool> is_solvable;
    std::string prediction;
    std::string answer;
    std::string unsolvable_reason;
    std::optional<std::string> error;
};

AgentResult solve_problem(Agent &agent, const std::string &question);
bool is_correct(const std::string &prediction, const std::string &answer);
GaiaEval evaluate_gaia_single(const GaiaProblem &problem, Agent &agent, const std::string &model);
std::map<std::string, std::vector<GaiaEval>> run_experiment(const std::vector<GaiaProblem> &problems, const std::vector<std::string> &models); 
std::vector<GaiaProblem> load_gaia_dataset(const std::string &filepath);
void print_results(const std::map<std::string, std::vector<GaiaEval>> &results);
