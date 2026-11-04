//TODO: Consolidate normal and format chat to one function

#include "ollama.hpp"

struct Message2 {
    std::string role;
    std::string content;

    std::string tool_name;
    json tool_calls = json::array();
};

struct Options { 
    bool think = false;
    bool stream = false;
    json tools = json::array();
    json format = nullptr;
};

json message_to_json(const Message2 &msg);
json chat(const std::string &model, const std::vector<Message2> &msg, const Options &options = {}); 

json message_to_json(const Message2 &msg) {
    json result = {{"role", msg.role}, {"content", msg.content}};

    if (!msg.tool_name.empty())
        result["tool_name"] = msg.tool_name;

    if (!msg.tool_calls.empty())
        result["tool_calls"] = msg.tool_calls;

    return result;
}

size_t write_callback(char *data, size_t size, size_t nmemb, std::string *response) {
    size_t bytes = size * nmemb;
    response->append(data, bytes);
    return bytes;
}


json chat(const std::string &model, const std::vector<Message2> &messages, const Options &options) {
    // TODO: Streaming is not supported currently
    if (options.stream)
        throw std::runtime_error("Streaming is not implemented yet");

    // Construct JSON request 
    json request = {
        {"model", model},
        {"stream", options.stream},
        {"think", options.think},
        {"messages", json::array()}
    };

    for (const Message2& msg : messages)
        request["messages"].push_back(message_to_json(msg));

    if (!options.tools.empty())
        request["tools"] = options.tools;

    if (!options.format.is_null())
        request["format"] = options.format;

    std::string request_body = request.dump();

    CURL *curl = curl_easy_init();
    if (!curl) 
        throw std::runtime_error("Failed to initialise CURL");
    
    std::string response_body;
    
    // Send/recv the HTTP request
    curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/chat");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    CURLcode result = curl_easy_perform(curl);

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(result));

    if (status_code < 200 || status_code >= 300)
        throw std::runtime_error("Ollama returned HTTP " + 
                                 std::to_string(status_code) +
                                 ": " + response_body);

    return json::parse(response_body);
}

json normal_chat(const std::string &model, 
        const std::vector<Message> &messages,
        const std::vector<json> &tools)  {
    // Construct the JSON request 
    json request;

    request["model"] = model;
    request["stream"] = false;
    request["think"] = false;

    request["messages"] = json::array();
    request["tools"] = json::array();
    
    for (const json &tool : tools) 
        request["tools"].push_back(tool);

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
    return response;
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
    return response;
}


std::variant<std::vector<json>, std::string> web_search_tool(const std::string &query, 
        int max_results, 
        const std::string &categories, 
        const std::string &time_range, 
        const std::string &language) {
    try {
        CURL *curl = curl_easy_init();

        if (!curl) {
            throw std::runtime_error("Failed to initialise CURL");
        }

        std::string response_body;

        auto encode = [&](const std::string &value) {
            char *escaped = curl_easy_escape(curl, value.c_str(), 0);
            if (!escaped) {
                throw std::runtime_error("Failed to encode search parameter");
            }

            std::string result(escaped);
            curl_free(escaped);
            return result;
        };

        std::string url = "http://localhost:8888/search?q=" + encode(query);
        url += "&format=json";
        url += "&categories=" + encode(categories);

        if (!time_range.empty())
            url += "&time_range=" + encode(time_range);

        if (!language.empty())
            url += "&language=" + encode(language);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode result = curl_easy_perform(curl);

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
        curl_easy_cleanup(curl);

        if (result != CURLE_OK)
            throw std::runtime_error(curl_easy_strerror(result));

        if (status_code != 200)
            throw std::runtime_error("SearXNG returned HTTP " + std::to_string(status_code));

        json response = json::parse(response_body);
        std::vector<json> results = response.at("results");

        if (max_results < 0)
            throw std::invalid_argument("max_results cannot be negative");

        if (results.size() > static_cast<size_t>(max_results))
            results.resize(max_results);

        return results;
    }
    catch (const std::exception &e) {
        return "Error: Search failed - " + std::string(e.what()); 
    }
}
