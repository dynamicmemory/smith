#include "ollama.hpp"

json message_to_json(const Message &msg) {
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

json chat(const std::string &model, const std::vector<Message> &messages, const Options &options) {
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

    for (const Message& msg : messages)
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

