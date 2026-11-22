#include "context_optimizer.hpp"

#include "optimizer_callback.hpp"

ContextOptimizer::ContextOptimizer(
        LlmClient &llm_client,
        int token_threshold,
        bool enable_compaction, 
        bool enable_summarization,
        int keep_recent
        ) : 
        llm_client(llm_client),
        token_threshold(token_threshold),
        enable_compaction(enable_compaction),
        enable_summarization(enable_summarization),
        keep_recent(keep_recent){}

std::optional<LlmResponse> ContextOptimizer::operator()(
        ExecutionContext &context, 
        LlmRequest &request) {

    int tokens = estimate_tokens(request);
    if (tokens <= token_threshold)
        return std::nullopt;

    if (enable_compaction) {
        apply_compaction(context, request);

        tokens = estimate_tokens(request);
        if (tokens <= token_threshold)
            return std::nullopt;

    }

    if (enable_summarization)
        apply_summarization(context, request, llm_client, keep_recent);

    return std::nullopt;
}
