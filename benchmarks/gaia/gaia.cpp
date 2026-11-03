#include "gaia.hpp"
#include <iostream>
#include <fstream>

bool is_correct(const std::string prediction, const std::string answer) {
    if (prediction.empty())
        return false;

    // TODO: Extract to to_lower and trim functions, for now dirty ass LLM code 
    auto normalise = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {return std::tolower(c); });

        auto first = s.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) 
            return std::string{};

        auto last = s.find_last_not_of(" \t\n\r");
        return s.substr(first, last - first + 1);
    };
    return normalise(prediction) == normalise(answer);
}

GaiaOutput solve_problem(const std::string &model, const std::string &question) {
    std::vector<Message> messages;
    const std::string gaia_prompt = 
        "You are a general AI assistant. I will ask you a question. "
        "First, determine if you can solve this problem with your current capabilities and set "
        "“is_solvable” accordingly. "
        "If you can solve it, set “is_solvable” to true and provide your answer in “final_answer”. "
        "If you cannot solve it, set “is_solvable” to false and explain why in “unsolvable_reason”. "
        "Your final answer should be a number OR as few words as possible OR a comma- "
        "separated list of numbers and/or strings. "
        "If you are asked for a number, don’t use a comma to write your number; also don’t "
        "use units such as $ or a percent sign unless specified otherwise. "
        "If you are asked for a string, don’t use articles, neither abbreviations (e.g., for cities), "
        "and write the digits in plain text unless specified otherwise. "
        "If you are asked for a comma-separated list, apply the above rules depending on "
        "whether the element is a number or a string. ";

    messages.push_back({"system", gaia_prompt, ""});
    messages.push_back({"user", question, ""});
    json schema = {
        {"type", "object"},
        {"properties", {
            {"is_solvable", {{"type", "boolean"}}},
            {"unsolvable_reason", {{"type", "string"}}},
            {"final_answer", {{"type", "string"}}}
        }},
        {"required", {"is_solvable", "unsolvable_reason", "final_answer"}},
        {"additionalProperties", false}
    };

    json response = format_chat(model, messages, schema);

    std::string done_reason = response.value("done_reason", "");
    std::string content = response["message"].value("content", "");
    json result = json::parse(content);

    // TODO: This check is essentially useless and if it fails its before here, FIX
    if (result.empty())
        return {false, "The model refused to answer, or timedout. Done reason: " + done_reason, ""};

    GaiaOutput output = {
        .is_solvable = result["is_solvable"].get<bool>(),
        .unsolvable_reason = result["unsolvable_reason"].get<std::string>(),
        .final_answer = result["final_answer"].get<std::string>()
    };

    return output;
}

std::map<std::string, std::vector<GaiaEval>> run_experiment(const std::vector<GaiaProblem> &problems,
                                                             const std::vector<std::string> &models) {
    std::map<std::string, std::vector<GaiaEval>> results;

    if (problems.empty()) {
        std::cerr << "Problems is empty" << '\n';
        return results;
    }

    for (const auto &model: models) 
        results[model] = {};

    for (const auto &problem: problems) {
        for (const auto &model: models) {
            GaiaEval result = evaluate_gaia_single(problem, model);

            // Print each Q as its answered
            std::cout << "[" << model << "] " << "Task " << problem.task_id << 
                " | " << (result.correct ? "CORRECT" : "INCORRECT") << '\n';

            results[model].push_back(result);
        }
    }
    return results;
}

GaiaEval evaluate_gaia_single(const GaiaProblem &problem, const std::string &model) {
    try {
        GaiaOutput output = solve_problem(model, problem.question);

        return {
            .task_id = problem.task_id,
            .model = model,
            .correct = is_correct(output.final_answer, problem.answer),
            .is_solvable = output.is_solvable,
            .prediction = output.final_answer,
            .answer = problem.answer,
            .unsolvable_reason = output.unsolvable_reason, 
            .error = std::nullopt
        };
    }
    catch (const std::exception &e) {
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

std::vector<GaiaProblem> load_gaia_dataset(const std::string &filepath) {
    std::vector<GaiaProblem> problems;
    std::ifstream file(filepath);
    std::string line;

    while (std::getline(file, line)) {
        json row = json::parse(line);

        problems.push_back({
                row["task_id"], 
                row["Question"],
                row["Final answer"]
                });
    }
    return problems;
}

void print_results(const std::map<std::string, std::vector<GaiaEval>> &results) {
    for (const auto &[model, evaluations] : results) {
        int correct = 0;

        std::cout << "\n=== " << model << " ===\n";

        for (const auto &result : evaluations) {
            std::cout 
                << "Task: " << result.task_id << '\n'
                << "Solvable: " << (result.is_solvable ? "Yes" : "No") << '\n'
                << "Prediction: " << result.prediction << '\n'
                << "Expected: " << result.answer << '\n'
                << "Correct: " << (result.correct ? "Yes" : "No") << "\n\n";

            if (result.correct)
                ++correct;
        }

        double accuracy = evaluations.empty() ? 0.0 : 100.0 * correct / evaluations.size();
        std::cout << "Accuracy: " << accuracy << "% (" << correct << "/" << 
            evaluations.size() << ")\n";
    }
}
