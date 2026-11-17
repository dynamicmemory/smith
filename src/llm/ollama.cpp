#include "ollama.hpp"
#include <curl/curl.h>
#include <memory>
#include <stdexcept>
#include <string>

size_t write_callback(char *data, size_t size, size_t nmemb, std::string *response) {
    size_t bytes = size * nmemb;
    response->append(data, bytes);
    return bytes;
}

Json ollama_post(const std::string &url, const Json &payload) {
    std::string request_body = payload.dump();
    std::string response_body;

    using CurlHandle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
    CurlHandle curl(curl_easy_init(), curl_easy_cleanup);

    if (!curl)
        throw std::runtime_error("Failed to initialise CURL");

    curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    using HeaderHandle = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;
    HeaderHandle header_guard(headers, curl_slist_free_all);

    auto set_option = [&](CURLoption option, auto value) {
        CURLcode code = curl_easy_setopt(curl.get(), option, value);

        if (code != CURLE_OK)
            throw std::runtime_error(curl_easy_strerror(code));
    };

    set_option(CURLOPT_URL, url.c_str());
    set_option(CURLOPT_HTTPHEADER, headers);
    set_option(CURLOPT_POSTFIELDS, request_body.c_str());
    set_option(CURLOPT_WRITEFUNCTION, write_callback);
    set_option(CURLOPT_WRITEDATA, &response_body);
    set_option(CURLOPT_TIMEOUT, 1800L);

    CURLcode result = curl_easy_perform(curl.get());

    if (result != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(result));

    long status_code = 0;

    result = curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &status_code);

    if (result != CURLE_OK)
        throw std::runtime_error(curl_easy_strerror(result));

    if (status_code < 200 || status_code >= 300)
        throw std::runtime_error(
            "HTTP " + std::to_string(status_code) + ": " + response_body
        );

    return Json::parse(response_body);
}
