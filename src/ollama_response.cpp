#include "ollama_response.hpp"

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>

std::string parse_ollama_response(std::string_view body) {
    using nlohmann::json;

    try {
        const json response = json::parse(body);

        if (!response.contains("message")
            || !response["message"].is_object()
            || !response["message"].contains("content")
            || !response["message"]["content"].is_string()) {
            throw std::runtime_error(
                "Ollama response is missing message.content"
            );
        }

        return response["message"]["content"].get<std::string>();
    } catch (const json::exception& error) {
        throw std::runtime_error(
            "Invalid JSON response from Ollama: "
            + std::string(error.what())
        );
    }
}
