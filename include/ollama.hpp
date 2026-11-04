#pragma once

#include <string>
#include <vector> 
#include <variant>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

using json = nlohmann::json;

struct Message {
    std::string role;
    std::string content;
    std::string tool_name;
};

json normal_chat(const std::string &model, 
                 const std::vector<Message> &messages,
                 const std::vector<json> &tools = json::array());
json format_chat(const std::string &model, const std::vector<Message> &messages, const json &schema);

std::variant<std::vector<json>, std::string> web_search_tool(const std::string &query, int max_results, 
                             const std::string &categories = "general",
                             const std::string &time_range = "", 
                             const std::string &language = ""
                             );
