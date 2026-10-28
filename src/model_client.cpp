#include "smith/model_client.hpp"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace {

using json = nlohmann::json;

class CurlGlobal {
public:
    CurlGlobal() {
        const CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
        if (result != CURLE_OK) {
            throw std::runtime_error("Failed to initialise libcurl");
        }
    }
    ~CurlGlobal() { curl_global_cleanup(); }
    CurlGlobal(const CurlGlobal&) = delete;
    CurlGlobal& operator=(const CurlGlobal&) = delete;
};

CurlGlobal& curl_global() {
    static CurlGlobal instance;
    return instance;
}

struct StreamContext {
    std::string buffer;
    ModelResponse response;
    ModelClient::StreamCallback on_chunk;
    std::string error;
    bool failed = false;
};

void process_line(const std::string& line, StreamContext& stream) {
    if (line.empty()) return;

    const json chunk = json::parse(line);
    if (chunk.contains("error")) {
        stream.error = chunk["error"].get<std::string>();
        stream.failed = true;
        return;
    }

    if (chunk.contains("message") && chunk["message"].is_object()) {
        const auto& message = chunk["message"];

        if (message.contains("content") && message["content"].is_string()) {
            const std::string text = message["content"].get<std::string>();
            stream.response.content += text;
            if (!text.empty() && stream.on_chunk) stream.on_chunk(text);
        }

        // Ollama streams tool calls as structured data, usually in one chunk.
        // Replace rather than append in case a backend repeats the full array.
        if (message.contains("tool_calls") && message["tool_calls"].is_array()) {
            stream.response.tool_calls.clear();
            for (const auto& call : message["tool_calls"]) {
                if (!call.contains("function") || !call["function"].is_object()) continue;
                const auto& function = call["function"];
                if (!function.contains("name") || !function["name"].is_string()) continue;
                ToolCall parsed;
                parsed.name = function["name"].get<std::string>();
                if (function.contains("arguments")) {
                    parsed.arguments = function["arguments"];
                    if (parsed.arguments.is_string()) {
                        try { parsed.arguments = json::parse(parsed.arguments.get<std::string>()); }
                        catch (...) { parsed.arguments = json::object(); }
                    }
                }
                stream.response.tool_calls.push_back(std::move(parsed));
            }
        }
    }
}

size_t receive_stream(char* data, size_t size, size_t count, void* user_data) {
    const size_t bytes = size * count;
    auto& stream = *static_cast<StreamContext*>(user_data);
    try {
        stream.buffer.append(data, bytes);
        size_t newline;
        while ((newline = stream.buffer.find('\n')) != std::string::npos) {
            const std::string line = stream.buffer.substr(0, newline);
            stream.buffer.erase(0, newline + 1);
            process_line(line, stream);
            if (stream.failed) return 0;
        }
        return bytes;
    } catch (const std::exception& error) {
        stream.error = error.what();
        stream.failed = true;
        return 0;
    }
}

} // namespace

ModelClient::ModelClient(std::string model, std::string base_url)
    : model_(std::move(model)), endpoint_(std::move(base_url)) {
    if (model_.empty()) throw std::invalid_argument("Model name cannot be empty");
    while (!endpoint_.empty() && endpoint_.back() == '/') endpoint_.pop_back();
    endpoint_ += "/api/chat";
}

ModelResponse ModelClient::chat(
    const std::vector<ChatMessage>& messages,
    const nlohmann::json& tools,
    StreamCallback on_chunk
) const {
    if (messages.empty()) throw std::invalid_argument("Chat history cannot be empty");
    (void)curl_global();

    CURL* raw_handle = curl_easy_init();
    if (!raw_handle) throw std::runtime_error("Failed to create HTTP client");
    struct HandleGuard { CURL* handle; ~HandleGuard() { curl_easy_cleanup(handle); } } handle_guard{raw_handle};

    json json_messages = json::array();
    for (const auto& message : messages) {
        if (message.role != "system" && message.role != "user" &&
            message.role != "assistant" && message.role != "tool") {
            throw std::invalid_argument("Invalid chat message role");
        }
        json item = {{"role", message.role}, {"content", message.content}};
        if (!message.tool_calls.is_null() && !message.tool_calls.empty()) item["tool_calls"] = message.tool_calls;
        if (!message.tool_name.empty()) item["tool_name"] = message.tool_name;
        json_messages.push_back(std::move(item));
    }

    json request = {
        {"model", model_},
        {"messages", json_messages},
        {"think", false},
        {"stream", true}
    };
    if (!tools.empty()) request["tools"] = tools;

    const std::string request_body = request.dump();
    StreamContext stream;
    stream.on_chunk = std::move(on_chunk);
    char error_buffer[CURL_ERROR_SIZE] = {};
    curl_slist* raw_headers = curl_slist_append(nullptr, "Content-Type: application/json");
    if (!raw_headers) throw std::runtime_error("Failed to allocate HTTP headers");
    struct HeaderGuard { curl_slist* headers; ~HeaderGuard() { curl_slist_free_all(headers); } } header_guard{raw_headers};

    curl_easy_setopt(raw_handle, CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(raw_handle, CURLOPT_HTTPHEADER, raw_headers);
    curl_easy_setopt(raw_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(raw_handle, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(raw_handle, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(request_body.size()));
    curl_easy_setopt(raw_handle, CURLOPT_WRITEFUNCTION, receive_stream);
    curl_easy_setopt(raw_handle, CURLOPT_WRITEDATA, &stream);
    curl_easy_setopt(raw_handle, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(raw_handle, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(raw_handle, CURLOPT_TIMEOUT, 1200L);
    curl_easy_setopt(raw_handle, CURLOPT_NOPROGRESS, 1L);

    const CURLcode result = curl_easy_perform(raw_handle);
    if (stream.failed) throw std::runtime_error("Failed to process Ollama stream: " + stream.error);
    if (result != CURLE_OK) {
        const std::string detail = error_buffer[0] ? error_buffer : curl_easy_strerror(result);
        throw std::runtime_error("Ollama request failed: " + detail);
    }
    long status = 0;
    curl_easy_getinfo(raw_handle, CURLINFO_RESPONSE_CODE, &status);
    if (status < 200 || status >= 300) throw std::runtime_error("Ollama returned HTTP " + std::to_string(status));

    if (!stream.buffer.empty()) process_line(stream.buffer, stream);
    if (stream.failed) throw std::runtime_error("Failed to process Ollama stream: " + stream.error);
    return stream.response;
}
