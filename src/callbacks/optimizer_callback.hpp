#pragma once 

#include "agent.hpp"

BeforeLlmCallback create_optimizer_callback(
        OptimizationFunction apply_optimization, int threshold = 50000);


void apply_sliding_window(ExecutionContext &context, LlmRequest &request, int window = 20);
void apply_compaction(ExecutionContext &context, LlmRequest &request);
void apply_summarization(ExecutionContext &context, LlmRequest &request, LlmClient &llm_client, int keep_recent=5);
int count_tokens(const ExecutionContext &context);
int estimate_tokens(const LlmRequest &request);
std::optional<std::string> generate_summary(const std::string &history, LlmClient &llm_client);
