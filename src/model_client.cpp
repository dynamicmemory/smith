#include "smith/model_client.hpp"
#include "ollama_response.hpp"

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

    ~CurlGlobal() {
        curl_global_cleanup();
    }

    CurlGlobal(const CurlGlobal&) = delete;
    CurlGlobal& operator=(const CurlGlobal&) = delete;
};

CurlGlobal& curl_global() {
    static CurlGlobal instance;
    return instance;
}


struct StreamContext {
    std::string buffer;
    std::string response;
    ModelClient::StreamCallback on_chunk;
    std::string error;
    bool failed = false;
};

size_t receive_stream(char* data, size_t size, size_t count, void* user_data) {
    const size_t bytes = size * count;
    auto& stream = *static_cast<StreamContext*>(user_data);

    try {
        stream.buffer.append(data, bytes);

        size_t newline;

        while ((newline = stream.buffer.find('\n'))
               != std::string::npos) {
            const std::string line = stream.buffer.substr(0, newline);
            stream.buffer.erase(0, newline + 1);

            if (line.empty()) {
                continue;
            }

            const auto chunk = nlohmann::json::parse(line);

            if (chunk.contains("error")) {
                stream.error = chunk["error"].get<std::string>();
                stream.failed = true;
                return 0;
            }

            if (chunk.contains("message")
                && chunk["message"].is_object()
                && chunk["message"].contains("content")
                && chunk["message"]["content"].is_string()) {
                const std::string text =
                    chunk["message"]["content"].get<std::string>();

                stream.response += text;

                if (!text.empty() && stream.on_chunk) {
                    stream.on_chunk(text);
                }
            }
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
    : model_(std::move(model)),
      endpoint_(std::move(base_url))
{
    if (model_.empty()) {
        throw std::invalid_argument("Model name cannot be empty");
    }

    while (!endpoint_.empty() && endpoint_.back() == '/') {
        endpoint_.pop_back();
    }

    endpoint_ += "/api/chat";
}

std::string ModelClient::chat(
    const std::vector<ChatMessage>& messages,
    StreamCallback on_chunk
) const {
    if (messages.empty()) {
        throw std::invalid_argument("Chat history cannot be empty");
    }

    (void)curl_global();

    CURL* raw_handle = curl_easy_init();

    if (raw_handle == nullptr) {
        throw std::runtime_error("Failed to create HTTP client");
    }

    struct HandleGuard {
        CURL* handle;
        ~HandleGuard() { curl_easy_cleanup(handle); }
    } handle_guard{raw_handle};

    nlohmann::json json_messages = nlohmann::json::array();

    for (const auto& message : messages) {
        if (message.role != "system"
            && message.role != "user"
            && message.role != "assistant") {
            throw std::invalid_argument("Invalid chat message role");
        }

        json_messages.push_back({
            {"role", message.role},
            {"content", message.content}
        });
    }

    const nlohmann::json request = {
        {"model", model_},
        {"messages", json_messages},
        {"think", false},
        {"stream", true}
    };

    const std::string request_body = request.dump();
    StreamContext stream{
        .on_chunk = std::move(on_chunk)
    };
    char error_buffer[CURL_ERROR_SIZE] = {};

    curl_slist* raw_headers = curl_slist_append(
        nullptr, "Content-Type: application/json"
    );

    if (raw_headers == nullptr) {
        throw std::runtime_error("Failed to allocate HTTP headers");
    }

    struct HeaderGuard {
        curl_slist* headers;
        ~HeaderGuard() { curl_slist_free_all(headers); }
    } header_guard{raw_headers};

    curl_easy_setopt(raw_handle, CURLOPT_URL, endpoint_.c_str());
    curl_easy_setopt(raw_handle, CURLOPT_HTTPHEADER, raw_headers);
    curl_easy_setopt(raw_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(raw_handle, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(
        raw_handle,
        CURLOPT_POSTFIELDSIZE_LARGE,
        static_cast<curl_off_t>(request_body.size())
    );
    curl_easy_setopt(raw_handle, CURLOPT_WRITEFUNCTION, receive_stream);
    curl_easy_setopt(raw_handle, CURLOPT_WRITEDATA, &stream);
    curl_easy_setopt(raw_handle, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(raw_handle, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(raw_handle, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(raw_handle, CURLOPT_NOPROGRESS, 1L);

    const CURLcode result = curl_easy_perform(raw_handle);

    if (stream.failed) {
        throw std::runtime_error(
            "Failed to process Ollama stream: " + stream.error
        );
    }

    if (result != CURLE_OK) {
        const std::string detail = error_buffer[0] != '\0'
            ? error_buffer
            : curl_easy_strerror(result);

        throw std::runtime_error("Ollama request failed: " + detail);
    }

    long status = 0;
    curl_easy_getinfo(raw_handle, CURLINFO_RESPONSE_CODE, &status);

    if (status < 200 || status >= 300) {
        throw std::runtime_error(
            "Ollama returned HTTP " + std::to_string(status)
        );
    }

    // Ollama normally terminates each JSON object with a newline.
    // Process a final unterminated line defensively.
    if (!stream.buffer.empty()) {
        const auto chunk = nlohmann::json::parse(stream.buffer);

        if (chunk.contains("error")) {
            throw std::runtime_error(
                chunk["error"].get<std::string>()
            );
        }

        if (chunk.contains("message")
            && chunk["message"].is_object()
            && chunk["message"].contains("content")
            && chunk["message"]["content"].is_string()) {
            const std::string text =
                chunk["message"]["content"].get<std::string>();

            stream.response += text;

            if (!text.empty() && stream.on_chunk) {
                stream.on_chunk(text);
            }
        }
    }

    return stream.response;
}
