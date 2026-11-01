//TODO: Consolidate normal and format chat to one function

#include "ollama.hpp"

size_t write_callback(char *data, size_t size, size_t nmemb, std::string *response) {
    size_t bytes = size * nmemb;
    response->append(data, bytes);
    return bytes;
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
