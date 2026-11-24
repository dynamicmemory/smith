#include "memory_manager.hpp"
#include "rag.hpp"

#include <stdexcept>
#include <utility>
#include <atomic>
#include <chrono>
#include <stdexcept>
#include <type_traits>
#include <utility>

MemoryManager::MemoryManager(MemoryStore &store, LlmClient &llm, const std::string &embedding_model)
        : 
        store(store), llm(llm), embedding_model(embedding_model) {}

void MemoryManager::add_memory(Memory memory) {
    auto embeddings = get_embeddings({memory.summary}, embedding_model);

    if (embeddings.empty())
        throw std::runtime_error("Failed to generate memory embedding");

    memory.embedding = std::move(embeddings.front());

    store.add(memory);
}

std::vector<Memory> MemoryManager::search_memories(const std::string &query, size_t top_k) {
    auto embeddings = get_embeddings({query}, embedding_model);

    if (embeddings.empty())
        throw std::runtime_error("Failed to generate query embedding");

    return store.search(embeddings.front(), top_k);
}

// TODO: Break this function up into format_history, parse_memories, extract_memories
void MemoryManager::extract_memories(const std::vector<Event> &events) {
    // Convert execution history into readable text.
    std::string history;

    for (const auto &event : events) {
        history += "\nAuthor: " + event.author + "\n";

        for (const auto &item : event.content) {
            std::visit([&](const auto &content) {
                using T = std::decay_t<decltype(content)>;

                if constexpr (std::is_same_v<T, Message>) {
                    history += content.role + ": " + content.content + "\n";
                }
                else if constexpr (std::is_same_v<T, ToolCall>) {
                    history += "Tool call: " + content.name +
                               "\nArguments: " + content.arguments.dump() + "\n";
                }
                else if constexpr (std::is_same_v<T, ToolResult>) {
                    history += "Tool result: " + content.name + "\n";

                    for (const auto &result : content.content)
                        history += result.dump() + "\n";
                }
            }, item);
        }
    }

    if (history.empty())
        return;

    // Ask the LLM to extract useful, reusable memories.
    LlmRequest request;

    request.instructions.push_back(
        "Extract useful, reusable information from the supplied execution history.\n"
        "Identify facts, preferences, knowledge, decisions, and problem-solving "
        "information that could be useful in future interactions.\n"
        "Only extract information supported by the history. Do not invent facts.\n"
        "Avoid transient details that have no likely future value.\n\n"
        "Return ONLY valid JSON in this exact structure:\n"
        "{\n"
        "  \"memories\": [\n"
        "    {\n"
        "      \"summary\": \"Concise description for semantic retrieval\",\n"
        "      \"content\": \"The actual information being remembered\",\n"
        "      \"category\": \"Broad classification\"\n"
        "    }\n"
        "  ]\n"
        "}\n"
        "If nothing is worth remembering, return {\"memories\": []}."
    );

    request.contents.push_back(Message{
        .role = "user",
        .content = history
    });

    LlmResponse response = llm.generate(request);

    if (response.error_message.has_value())
        throw std::runtime_error(
            "Memory extraction failed: " + *response.error_message
        );

    // Extract the assistant's response.
    std::string output;

    for (const auto &item : response.contents) {
        if (const auto *message = std::get_if<Message>(&item)) {
            if (message->role == "assistant")
                output += message->content;
        }
    }

    if (output.empty())
        throw std::runtime_error("Memory extraction returned no content");

    // Remove Markdown code fences if the model included them.
    size_t start = output.find_first_not_of(" \t\n\r");
    if (start != std::string::npos)
        output = output.substr(start);

    if (output.starts_with("```")) {
        size_t newline = output.find('\n');
        if (newline != std::string::npos)
            output = output.substr(newline + 1);

        size_t end = output.find_last_not_of(" \t\n\r");
        if (end != std::string::npos)
            output = output.substr(0, end + 1);

        if (output.ends_with("```"))
            output.erase(output.size() - 3);
    }

    Json parsed;

    try {
        parsed = Json::parse(output);
    }
    catch (const Json::exception &e) {
        throw std::runtime_error(
                "Memory extraction returned invalid JSON: " +
                std::string(e.what()) + "\nOutput: " + output
                );
    }

    // Json parsed;
    //
    // try {
    //     parsed = Json::parse(output);
    // }
    // catch (const Json::exception &e) {
    //     throw std::runtime_error(
    //         "Memory extraction returned invalid JSON: " +
    //         std::string(e.what())
    //     );
    // }

    const auto &extracted = parsed.at("memories");

    if (!extracted.is_array())
        throw std::runtime_error("Extracted memories must be an array");

    static std::atomic<uint64_t> next_id{0};

    // Convert extracted records into Memory objects and store them.
    for (const auto &item : extracted) {
        Memory memory;

        memory.id = "memory-" + std::to_string(
                std::chrono::system_clock::now().time_since_epoch().count()
                ) + "-" + std::to_string(next_id++);

        memory.summary = item.at("summary").get<std::string>();
        memory.content = item.at("content").get<std::string>();
        memory.category = item.at("category").get<std::string>();

        DuplicateCheckResult result = check_duplicate(memory);

        if (result.decision == MemoryDecision::add)
            add_memory(std::move(memory));
    }
}

DuplicateCheckResult MemoryManager::check_duplicate(
        const Memory &candidate,
        size_t top_k
        ) {
    auto existing_memories = search_memories(candidate.summary, top_k);

    // Nothing exists to compare against.
    if (existing_memories.empty())
        return {MemoryDecision::add, "No existing memories found."};

    Json candidates = Json::array();

    for (const auto &memory : existing_memories) {
        candidates.push_back({
                {"summary", memory.summary},
                {"content", memory.content},
                {"category", memory.category}
                });
    }

    LlmRequest request;

    request.instructions.push_back(
            "Determine whether the proposed memory contains information "
            "already captured by any existing memory.\n"
            "Only consider memories duplicates when they convey substantially "
            "the same information. Related but distinct information is not a duplicate.\n\n"
            "Return ONLY valid JSON:\n"
            "{\"decision\": \"ADD\" or \"SKIP\", \"reason\": \"explanation\"}"
            );

    request.contents.push_back(Message{
            .role = "user",
            .content =
            "Proposed memory:\n" + Json{
            {"summary", candidate.summary},
            {"content", candidate.content},
            {"category", candidate.category}
            }.dump(2) +
            "\n\nExisting memories:\n" + candidates.dump(2)
            });

    LlmResponse response = llm.generate(request);

    if (response.error_message)
        throw std::runtime_error(
                "Duplicate check failed: " + *response.error_message
                );

    std::string output;

    for (const auto &item : response.contents) {
        if (const auto *message = std::get_if<Message>(&item)) {
            if (message->role == "assistant")
                output += message->content;
        }
    }

    Json result = Json::parse(output);

    std::string decision = result.at("decision").get<std::string>();
    std::string reason = result.at("reason").get<std::string>();

    if (decision == "ADD")
        return {MemoryDecision::add, reason};

    if (decision == "SKIP")
        return {MemoryDecision::skip, reason};

    throw std::runtime_error("Invalid memory decision: " + decision);
}
