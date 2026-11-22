#pragma once

#include "llm_client.hpp"

class ContextOptimizer {
public:
    ContextOptimizer(
            LlmClient &llm_client, int token_threshold=1000,
            bool enable_compaction=true,
            bool enable_summarization=true,
            int keep_recent=5
    );

    std::optional<LlmResponse> operator()(ExecutionContext &context, LlmRequest &request);

private:
    LlmClient llm_client;
    int token_threshold;
    bool enable_compaction;
    bool enable_summarization;
    int keep_recent;
};
