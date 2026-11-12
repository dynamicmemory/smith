#include "web_search_tool.hpp"
#include <curl/curl.h>
#include <stdexcept>

WebSearchTool::WebSearchTool() : BaseTool(
        "web_search_tool", 
        "Seach the internet for recent or additional information",
        {
            {"type", "function"},
            {"function", {
                {"name", "web_search_tool"},
                {"description", "Search the internet for recent or additional information."},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                       {"query", {
                          {"type", "string"},
                          {"description", "The search query."}
                       }},
                       {"max_results", {
                          {"type", "integer"},
                          {"description", "Maximum number of results to return."}
                       }},
                       {"categories", {
                          {"type", "string"},
                          {"description", "Search categories."}
                       }},
                       {"time_range", {
                          {"type", "string"},
                          {"description", "Time range for results."}
                       }},
                       {"language", {
                          {"type", "string"},
                          {"description", "Language for results."}
                       }}
                   }},
                   {"required", {"query", "max_results"}}
                }}
           }}
       }
    ) {}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *user_data) {
    size_t bytes = size * nmemb;
    std::string *response = static_cast<std::string*>(user_data);
    response->append(static_cast<char*>(contents), bytes);
    return bytes;
}
    
ToolResult WebSearchTool::execute(ExecutionContext &, const Json &arguments) { 
    const std::string name = "web_search_tool";

    try {
        // Extract arguments
        const std::string query = arguments.at("query").get<std::string>();
        // const int max_results = arguments.at("max_results").get<int>();
        const int max_results = 1;
        const std::string categories = arguments.value("categories", "general");
        const std::string time_range = arguments.value("time_range", "");
        const std::string language = arguments.value("language", "");

        if (max_results < 1)
            throw std::invalid_argument("max_results must be atleast 1.");

        CURL *curl = curl_easy_init();

        if (!curl) 
            throw std::runtime_error("Failed to initialise CURL");

        std::string response_body;

        auto encode = [&](const std::string &value) {
            char *escaped = curl_easy_escape(curl, value.c_str(), 0);
            if (!escaped) {
                throw std::runtime_error("Failed to encode search parameter");
            }

            std::string result(escaped);
            curl_free(escaped);
            return result;
        };

        std::string url = "http://localhost:8888/search?q=" + encode(query);
        url += "&format=json";
        url += "&categories=" + encode(categories);

        if (!time_range.empty())
            url += "&time_range=" + encode(time_range);

        if (!language.empty())
            url += "&language=" + encode(language);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

        CURLcode result = curl_easy_perform(curl);

        long status_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
        curl_easy_cleanup(curl);

        if (result != CURLE_OK)
            throw std::runtime_error(curl_easy_strerror(result));

        if (status_code != 200)
            throw std::runtime_error("SearXNG returned HTTP " + std::to_string(status_code));

        Json response = Json::parse(response_body);
        std::vector<Json> results = response.at("results");

        if (results.size() > static_cast<size_t>(max_results))
            results.resize(max_results);

        return ToolResult {
            .tool_call_id = "",
            .name = name,
            .status = ToolStatus::success,
            .content = results 
        };
    }
    catch (const std::exception &e) {
        return ToolResult {
            .tool_call_id = "",
            .name = name,
            .status = ToolStatus::error,
            .content = {Json("Search failed: " + std::string(e.what()))}
        };
    }
}
