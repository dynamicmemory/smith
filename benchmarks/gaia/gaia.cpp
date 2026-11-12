#include "gaia.hpp"
#include "agents.hpp"
#include "llm_client.hpp"
#include "web_search_tool.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <stdexcept>

using Json = nlohmann::json;

bool is_correct(const std::string& prediction, const std::string& answer) {
    if (prediction.empty())
        return false;

    auto normalise = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) {
                return std::tolower(c);
            });

        auto first = s.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return std::string{};

        auto last = s.find_last_not_of(" \t\n\r");
        return s.substr(first, last - first + 1);
    };

    return normalise(prediction) == normalise(answer);
}

AgentResult solve_problem(Agent& agent, const std::string& question) {
    return agent.run(question);
}

std::map<std::string, std::vector<GaiaEval>> run_experiment(
    const std::vector<GaiaProblem>& problems,
    const std::vector<std::string>& models
) {
    std::map<std::string, std::vector<GaiaEval>> results;

    if (problems.empty()) {
        std::cerr << "Problems is empty\n";
        return results;
    }

    for (const auto& model : models) {
        LlmClient client(model);
        WebSearchTool search;

        Agent agent = create_gaia_agent(client, {&search});

        for (const auto& problem : problems) {
            GaiaEval result = evaluate_gaia_single(problem, agent, model);

            std::cout << "[" << model << "] Task "
                      << problem.task_id << " | ";

            if (result.error.has_value())
                std::cout << "ERROR";
            else
                std::cout << (result.correct ? "CORRECT" : "INCORRECT");

            std::cout << '\n' << std::flush;

            results[model].push_back(result);
        }
    }

    return results;
}

GaiaEval evaluate_gaia_single(
    const GaiaProblem& problem,
    Agent& agent,
    const std::string& model
) {
    try {
        AgentResult result = solve_problem(agent, problem.question);

        if (!result.output.has_value())
            throw std::runtime_error("Agent returned no output");

        const Json* output = std::get_if<Json>(&*result.output);

        if (output == nullptr || !output->is_object())
            throw std::runtime_error("Invalid structured output");

        GaiaOutput prediction{
            .is_solvable = output->at("is_solvable").get<bool>(),
            .unsolvable_reason = output->at("unsolvable_reason").get<std::string>(),
            .final_answer = output->at("final_answer").get<std::string>()
        };

        return {
            .task_id = problem.task_id,
            .model = model,
            .correct = is_correct(prediction.final_answer, problem.answer),
            .is_solvable = prediction.is_solvable,
            .prediction = prediction.final_answer,
            .answer = problem.answer,
            .unsolvable_reason = prediction.unsolvable_reason,
            .error = std::nullopt
        };
    }
    catch (const std::exception& e) {
        return {
            .task_id = problem.task_id,
            .model = model,
            .correct = false,
            .is_solvable = std::nullopt,
            .prediction = "",
            .answer = problem.answer,
            .unsolvable_reason = "",
            .error = e.what()
        };
    }
}

std::vector<GaiaProblem> load_gaia_dataset(const std::string& filepath) {
    std::vector<GaiaProblem> problems;
    std::ifstream file(filepath);

    if (!file.is_open())
        throw std::runtime_error("Could not open GAIA dataset: " + filepath);

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        Json row = Json::parse(line);

        problems.push_back({
            row["task_id"],
            row["Question"],
            row["Final answer"]
        });
    }

    std::cout << "Loaded " << problems.size() << " GAIA problems\n";

    return problems;
}

void print_results(
    const std::map<std::string, std::vector<GaiaEval>>& results
) {
    for (const auto& [model, evaluations] : results) {
        int correct = 0;

        std::cout << "\n=== " << model << " ===\n";

        for (const auto& result : evaluations) {
            std::cout << "Task: " << result.task_id << '\n';

            if (result.is_solvable.has_value())
                std::cout << "Solvable: "
                          << (*result.is_solvable ? "Yes" : "No") << '\n';
            else
                std::cout << "Solvable: N/A (error)\n";

            std::cout << "Prediction: " << result.prediction << '\n'
                      << "Expected: " << result.answer << '\n'
                      << "Correct: " << (result.correct ? "Yes" : "No") << '\n';

            if (result.error.has_value())
                std::cout << "Error: " << *result.error << '\n';

            if (!result.unsolvable_reason.empty())
                std::cout << "Unsolvable reason: "
                          << result.unsolvable_reason << '\n';

            std::cout << '\n';

            if (result.correct)
                ++correct;
        }

        double accuracy = evaluations.empty()
            ? 0.0
            : 100.0 * correct / evaluations.size();

        std::cout << "Accuracy: " << accuracy << "% ("
                  << correct << "/" << evaluations.size() << ")\n";
    }
}
