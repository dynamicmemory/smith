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

    std::string tool_name = "";
    json tool_calls = json::array();
};

struct Options { 
    bool think = false;
    bool stream = false;
    json tools = json::array();
    json format = nullptr;
};

json message_to_json(const Message &msg);
json chat(const std::string &model, const std::vector<Message> &msg, const Options &options = {}); 

std::variant<std::vector<json>, std::string> web_search_tool(const std::string &query, int max_results, 
                             const std::string &categories = "general",
                             const std::string &time_range = "", 
                             const std::string &language = ""
                             );
