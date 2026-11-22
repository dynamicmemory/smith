#include "search_compressor_callback.hpp"

#include "types.hpp"
#include "rag.hpp"
#include <string>
#include <variant>
#include <iostream>

int DEBUG = 0;

//TODO: Currently only implements for "web_search_tool" make that a list of tools 
//      to use rag context on besides just web requests
std::string extract_search_query(const ExecutionContext &context, const std::string &tool_call_id) {
    for (const Event &event : context.events()) {
        for (const ContentItem &item : event.content) {
            const auto *call = std::get_if<ToolCall>(&item);

            if (call && call->name == "web_search_tool" && call->tool_call_id == tool_call_id) 
                return call->arguments.value("query", "");
        }
    }

    return "";
}


// TODO: Again only uses "web_search_tool"
std::optional<ToolResult> search_compressor_callback(
        ExecutionContext &context, 
        const ToolResult &tool_result) {

    if (tool_result.name != "web_search_tool")
        return std::nullopt;

    if (tool_result.status != ToolStatus::success || tool_result.content.empty())
        return std::nullopt;

    // Convert the web search results into structured text for chunking
    std::string original_content = tool_result.content[0].dump(2);

    for (size_t i=1; i<tool_result.content.size(); ++i)
        original_content += "\n" + tool_result.content[i].dump(2);

    if (DEBUG) std::cout << "\n[Compressor] Original size: " 
                         << original_content.size() << " characters\n";

    if (original_content.size() < 2000) {
        if (DEBUG) std::cout << "\n[Compressor] Results too short; skipping.\n";

        return std::nullopt;
    }

    std::string query = extract_search_query(context, tool_result.tool_call_id);
    if (DEBUG) std::cout << "[Compressor] Query: " << query << '\n';

    if (query.empty())
        return std::nullopt;

    // Rag time 
    // 500 length char window with 50 char overlap window
    std::vector<std::string> chunks = fixed_length_chunking(original_content, 500, 50);
    auto embeddings = get_embeddings(chunks);
    // TODO: Retrieve the 5 most relevent, possibly tune lower or have llm request what it needs
    auto retrieved = vector_search(query, chunks, embeddings, 5);

    std::string compressed;
    for (const auto &[score, chunk] : retrieved) 
        compressed += chunk + "\n\n";

    if (DEBUG) std::cout << "\n[Compressor] Compressed size: " 
                         << compressed.size() << " characters\n";

    return ToolResult{ 
                   tool_result.tool_call_id, 
                   tool_result.name, 
                   ToolStatus::success, 
                   {Json(compressed)}
    };
}
