#pragma once

#include <string>
#include <vector> 
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
                 const json &tools = json::array());
json format_chat(const std::string &model, const std::vector<Message> &messages, const json &schema);
