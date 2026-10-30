#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream> 
#include <vector>
#include <string>
#include <cctype>
#include <algorithm>
#include <stdexcept>
#include <optional>
#include <cstdlib>
#include <map>

using json = nlohmann::json;

std::string model;

struct Message {
    std::string role;
    std::string content;
};

struct ExtractedInfo {
    std::string name;
    std::string email;
    std::optional<std::string> phone;
};

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

size_t write_callback(char *data, size_t size, size_t nmemb, std::string *response) {
    size_t bytes = size * nmemb;
    response->append(data, bytes);
    return bytes;
}

json normal_chat(const std::string &model, const std::vector<Message> &messages);
json format_chat(const std::string &model, const std::vector<Message> &messages, const json &schema);

GaiaOutput solve_problem(const std::string &model, const std::string &question);
bool is_correct(const std::string prediction, const std::string answer);
GaiaEval evaluate_gaia_single(const GaiaProblem &problem, const std::string &model);
std::map<std::string, std::vector<GaiaEval>> run_experiment(const std::vector<GaiaProblem> &problems, 
                                                            const std::vector<std::string> &models); 

int main() {
    std::vector<std::string> models = {
        "qwen3.5:9b",
        // "qwen3.6:27b"
    };

    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::vector<GaiaProblem> problems;
    std::vector<GaiaProblem> subset(problems.begin(), problems.begin() + 20);
    auto results = run_experiment(subset, models);

    curl_global_cleanup();
    return 0;
}


json normal_chat(const std::string &model, const std::vector<Message> &messages) {
    // Construct the JSON request 
    json request;

    request["model"] = model;
    request["stream"] = false;
    request["think"] = false;

    request["messages"] = json::array();

    for (const Message &msg: messages) 
        request["messages"].push_back({
                {"role", msg.role},
                {"content", msg.content}
                });

    // Convert the JSON obj into a string 
    std::string request_body = request.dump();

    // Prep the HHTP request 
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialise CURL");
    }

    std::string response_body;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/chat");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());

    // Tell CURL where to put the response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    // Send request
    CURLcode result = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(result));
    }

    json response = json::parse(response_body);
    // return response_body; if you want the full body return this 
    // return response;
    return response["message"]["content"].get<std::string>();
}


json format_chat(const std::string &model, const std::vector<Message> &messages, const json &schema) {
    // Construct the JSON request 
    json request;

    request["model"] = model;
    request["stream"] = false;
    request["think"] = false;

    request["messages"] = json::array();

    for (const Message &msg: messages) 
        request["messages"].push_back({
                {"role", msg.role},
                {"content", msg.content}
                });

    request["format"] = schema;

    // Convert the JSON obj into a string 
    std::string request_body = request.dump();

    // Prep the HHTP request 
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialise CURL");
    }

    std::string response_body;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/chat");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());

    // Tell CURL where to put the response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    // Send request
    CURLcode result = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(result));
    }

    json response = json::parse(response_body);
    return response; //if you want the full body return this 
    // return response["message"]["content"].get<std::string>();
}

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

    messages.push_back({"system", gaia_prompt});
    messages.push_back({"user", question});
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
