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

size_t receive_response(
    char* data,
    size_t size,
    size_t count,
    void* user_data
) {
    const size_t bytes = size * count;
    auto* response = static_cast<std::string*>(user_data);

    response->append(data, bytes);
    return bytes;
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

std::string ModelClient::chat(std::string_view message) const {
    (void)curl_global();

    CURL* raw_handle = curl_easy_init();

    if (raw_handle == nullptr) {
        throw std::runtime_error("Failed to create HTTP client");
    }

    struct HandleGuard {
        CURL* handle;

        ~HandleGuard() {
            curl_easy_cleanup(handle);
        }
    } handle_guard{raw_handle};

    json request = {
        {"model", model_},
        {"messages", json::array({
            {
                {"role", "user"},
                {"content", message}
            }
        })},
        {"stream", false}
    };

    const std::string request_body = request.dump();
    std::string response_body;
    char error_buffer[CURL_ERROR_SIZE] = {};

    curl_slist* raw_headers = curl_slist_append(
        nullptr, "Content-Type: application/json"
    );

    if (raw_headers == nullptr) {
        throw std::runtime_error("Failed to allocate HTTP headers");
    }

    struct HeaderGuard {
        curl_slist* headers;

        ~HeaderGuard() {
            curl_slist_free_all(headers);
        }
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
    curl_easy_setopt(raw_handle, CURLOPT_WRITEFUNCTION, receive_response);
    curl_easy_setopt(raw_handle, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(raw_handle, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(raw_handle, CURLOPT_CONNECTTIMEOUT, 3L);
    curl_easy_setopt(raw_handle, CURLOPT_TIMEOUT, 120L);

    const CURLcode result = curl_easy_perform(raw_handle);

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
            + ": " + response_body
        );
    }

    return parse_ollama_response(response_body);
    // try {
    //     const json response = json::parse(response_body);
        //
        // if (!response.contains("message")
            // || !response["message"].is_object()
            // || !response["message"].contains("content")
            // || !response["message"]["content"].is_string()) {
            // throw std::runtime_error(
            //     "Ollama response is missing message.content"
            // );
        // }
        //
    //     return response["message"]["content"].get<std::string>();
    // } catch (const json::exception& error) {
        // throw std::runtime_error(
        //     "Invalid JSON response from Ollama: "
        //     + std::string(error.what())
    //     // );
    // }
}
