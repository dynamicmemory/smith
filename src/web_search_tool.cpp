#include "web_search_tool.hpp"

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

    
ToolResult WebSearchTool::execute(ExecutionContext &context, const Json &arguments) { 
    const std::string query = arguments.at("query").get<std::string>();
    const int max_results = arguments.at("max_results").get<int>();
    const std::string categories = arguments.value("categories", "general");
    const std::string time_range = arguments.value("categories", "general");
    const std::string language = arguments.value("categories", "general");

    // Continue this tomorrow

    try {
        CURL *curl = curl_easy_init();

        if (!curl) {
            throw std::runtime_error("Failed to initialise CURL");
        }

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

        json response = json::parse(response_body);
        std::vector<json> results = response.at("results");

        if (max_results < 0)
            throw std::invalid_argument("max_results cannot be negative");

        if (results.size() > static_cast<size_t>(max_results))
            results.resize(max_results);

        return results;
    }
    catch (const std::exception &e) {
        return "Error: Search failed - " + std::string(e.what()); 
    }
}
