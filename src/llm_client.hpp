#pragma once

#include "llm_types.hpp"
#include <string>

/* Client for LLM API class using */
struct LlmClient {
    LlmClient(std::string model);

    LlmResponse generate(const LlmRequest &request);

private:
    std::string model;

    Json build_request(const LlmRequest &request);
    LlmResponse parse_response(const Json &response);
};
