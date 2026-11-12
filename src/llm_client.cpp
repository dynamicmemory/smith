#include "llm_client.hpp"
#include <utility>
#include <type_traits>
#include <curl/curl.h>

LlmClient::LlmClient(std::string model) : model(std::move(model)) {}

static size_t write_callback(char *data, size_t size, size_t nmemb, void *user_data) {
    size_t bytes = size * nmemb;
    auto *response = static_cast<std::string*>(user_data);
    response->append(data, bytes);

    return bytes;
}

LlmResponse LlmClient::generate(const LlmRequest &request) {
    try {
        Json body = build_request(request);

        std::string request_body = body.dump();
        std::string response_body;

        using CurlHandle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
        CurlHandle curl(curl_easy_init(), curl_easy_cleanup);

        if (!curl) 
            throw std::runtime_error("Failed to initialise CURL");

        // Send/recv the HTTP request
        curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        using HeaderHandle = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;
        HeaderHandle header_guard(headers, curl_slist_free_all);

        auto set_option = [&](CURLoption option, auto value) {
            CURLcode code = curl_easy_setopt(curl.get(), option, value);

            if (code != CURLE_OK)
                throw std::runtime_error(curl_easy_strerror(code));
        };

        set_option(CURLOPT_URL, "http://localhost:11434/api/chat");
        set_option(CURLOPT_HTTPHEADER, headers);
        set_option(CURLOPT_POSTFIELDS, request_body.c_str());
        set_option(CURLOPT_WRITEFUNCTION, write_callback);
        set_option(CURLOPT_WRITEDATA, &response_body);
        set_option(CURLOPT_TIMEOUT, 1800L);

        CURLcode result = curl_easy_perform(curl.get());

        if (result != CURLE_OK)
            throw std::runtime_error(curl_easy_strerror(result));

        long status_code = 0;

        result = curl_easy_getinfo(
            curl.get(), CURLINFO_RESPONSE_CODE, &status_code
        );

        if (result != CURLE_OK)
            throw std::runtime_error(curl_easy_strerror(result));

        if (status_code < 200 || status_code >= 300)
            throw std::runtime_error(
                "Ollama returned HTTP " + std::to_string(status_code) +
                ": " + response_body
            );
        return parse_response(Json::parse(response_body));
    }
    catch (const std::exception &e) {
        return LlmResponse {
            .contents = {},
            .error_message = e.what(),
            .usage_metadata = Json::object()
        };
    }
}

/* Builds the out going message to the model*/
Json LlmClient::build_request(const LlmRequest &request) {
    Json body;
    
    body["model"] = model;
    body["stream"] = false;
    body["think"] = false;    // Add config for this param
    body["messages"] = Json::array();

    for (const auto &instruction : request.instructions) 
        body["messages"].push_back({
                {"role", "system"},
                {"content", instruction}
        });

    for (const auto &item : request.contents) {
        std::visit([&](const auto &content) {
            using T = std::decay_t<decltype(content)>;

            // Parse what type of content is being sent
            if constexpr(std::is_same_v<T, Message>) {
                body["messages"].push_back({
                        {"role", content.role},
                        {"content", content.content}
                });
            }
            else if constexpr(std::is_same_v<T, ToolCall>) {
                Json tool_call = {
                   {"function", {
                       {"name", content.name},
                       {"arguments", content.arguments}
                    }}
                };
                 
                // If there has already been a tool call, add it to content for 
                // the next tool call too.
                if (!body["messages"].empty() && 
                     body["messages"].back().value("role", "") == "assistant") {

                    auto &last_message = body["messages"].back();

                    if (!last_message.contains("tool_calls"))
                        last_message["tool_calls"] = Json::array();

                    last_message["tool_calls"].push_back(tool_call);
                }
                else {

                    body["messages"].push_back({
                        {"role", "assistant"},
                        {"content", ""},
                        {"tool_calls", Json::array({tool_call})}
                    });
                }
            }
            else if constexpr(std::is_same_v<T, ToolResult>) {
                body["messages"].push_back({
                        {"role", "tool"}, 
                        {"tool_name", content.name},
                        {"content", content.content.empty() ? "" : content.content.front().dump()}
                });
            }
        }, item);
    }
    
    if (!request.tools.empty()) {
        body["tools"] = Json::array();

        for (const auto *tool : request.tools) 
            body["tools"].push_back(tool->tool_definition());
    }
    if (request.tool_choice.has_value())
        body["tool_choice"] = *request.tool_choice;

    return body;
}

/* Parses the returning response from the model*/
LlmResponse LlmClient::parse_response(const Json &response) {
    LlmResponse result;
    const auto &message = response.at("message");

    // Parse messages from the model
    if (message.contains("content") && !message["content"].get<std::string>().empty()) 
        result.contents.push_back(Message{
                .role = "assistant",
                .content = message["content"].get<std::string>()
        });

    // Parse tool calls from the model
    if (message.contains("tool_calls"))
        for (const auto &tool_call : message["tool_calls"]) 
            result.contents.push_back(ToolCall{
                    .tool_call_id = "",
                    .name = tool_call.at("function").at("name").get<std::string>(),
                    .arguments = tool_call.at("function").at("arguments")
            });
    
    result.usage_metadata = {
        {"input_tokens", response.value("prompt_eval_count", 0)},
        {"output_tokens", response.value("eval_count", 0)}
    };

    return result;
}
