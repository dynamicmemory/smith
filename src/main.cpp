#include <string>
#include <vector>
#include "types.hpp"
#include "ollama.hpp"
#include <curl/curl.h>
#include <cmath>
#include <utility>
#include <algorithm>
#include <filesystem>
#include <fstream>

#include <iostream>

const std::string model = "qwen3.5:9b";
const std::string embed_model = "nomic-embed-text";

/* Call ollama using an embedding model and returns the JSON response*/
Json call_embedding_model(std::vector<std::string> text, 
                       const std::string &model) {

    Json payload = {
        {"model", model},
        {"input", text}
    };
     
    return ollama_post("http://localhost:11434/api/embed", payload);
}

//TODO: add optional for vector of strings of just string for text, use check to wrap string in a vector if needed
std::vector<std::vector<float>> get_embeddings(std::vector<std::string> text, 
                                               const std::string &model) {
    // Check on text 
    Json response = call_embedding_model(text, model);
    std::vector<std::vector<float>> embeddings;

    for (const auto &embedding : response.at("embeddings"))
        embeddings.push_back(embedding.get<std::vector<float>>());

    return embeddings;
}

float who_sign_you_sign_cosine(const std::vector<float> &a, const std::vector<float> &b ) {
    if (a.size() != b.size())
        throw std::invalid_argument("Embedding dimensions do not match");

    float dot_product = 0.0f;
    float mag_a = 0.0f;
    float mag_b = 0.0f;

    for (size_t i=0; i<a.size(); ++i) {
        dot_product += a[i] * b[i];
        mag_a += a[i] * a[i];
        mag_b += b[i] * b[i];
    }

    mag_a = std::sqrt(mag_a);
    mag_b = std::sqrt(mag_b);

    if (mag_a == 0.0f || mag_b == 0.0f)
        throw std::invalid_argument("Cannot calculate cosine similarity for zero vectors");

    return dot_product / (mag_a * mag_b);
}

std::vector<std::string> fixed_length_chunking(const std::string &text, int chunk_size, int overlap) {
    if (chunk_size <= 0 || overlap <= 0) 
        throw std::invalid_argument("chunk_size and overlap must be greater than 0");
    else if (overlap > chunk_size)
        throw std::invalid_argument("chunk_size must be larger than overlap");

    std::vector<std::string> chunks;

    for (size_t start=0; start<text.size(); start += chunk_size - overlap) {
        chunks.push_back(text.substr(start, chunk_size));

        if ( (start + chunk_size) > text.size() )
            break;
    }
    return chunks;
}

std::vector<std::pair<float, std::string>> vector_search(const std::string &query,
                                       const std::vector<std::string> &chunks,
                                       const std::vector<std::vector<float>> &chunk_embeddings,
                                       int top_k) {
    std::vector<std::vector<float>> query_embedding = get_embeddings({query}, embed_model);
    std::vector<std::pair<float, size_t>> scores;
 
    size_t i = 0;
    for (const auto &chunk : chunk_embeddings) {
        float similarity = who_sign_you_sign_cosine(query_embedding[0], chunk);
        scores.push_back({similarity, i});

        ++i;
    }
    
    std::sort(scores.begin(), scores.end(), [](const auto &a, const auto &b) {
        return a.first > b.first;
        }
    );

    std::vector<std::pair<float, std::string>> results;
    for (size_t i=0; i<top_k; ++i)
        results.push_back({scores[i].first, chunks[scores[i].second]});
        
    return results;
}

/* List files and directories in the given path*/
std::string list_files_tool(const std::string &path) {
    namespace fs = std::filesystem;
    fs::path directory(path); 

    if (!fs::exists(directory))
        return "Path not found: " + path;

    if (!fs::is_directory(directory))
        return "Not a directory: " + path;

    std::vector<std::string> dirs;
    std::vector<std::string> files;
    
    for (const auto &item : fs::directory_iterator(directory)) {
        std::string filename = item.path().filename().string();

        // TODO: We are skipping hidden files for future refference
        if (filename.starts_with('.'))
            continue;

        if (item.is_directory())
            dirs.push_back(filename+"/");
        else 
            files.push_back(filename);
    }

    std::sort(dirs.begin(), dirs.end(), [](const auto &a, const auto &b) {
        return a < b; });
    std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) {
        return a < b; });

    std::string result = "Directory: " + path + "\n";
    for (const auto &item : dirs) 
        result += item + '\n';
    for (const auto &item : files) 
        result += item + '\n';

    return result;
}

/* Read file content. Supports txt, py, json, md, csv, xlsx, cpp, c?*/
std::string read_file_tool(const std::string &path, int start_line=1, int end_line=-1);
std::string read_text_file(const std::string &path, int start_line, int end_line);
std::string read_csv_file(const std::string &path);
std::vector<std::string> TEXT_EXTENSIONS = { 
    ".txt", ".py", ".js", ".json", ".md", ".html",
    ".css", ".xml", ".yaml", ".yml", ".log", ".sh",".cpp", ".c"
};

std::string read_file_tool(const std::string &path, int start_line, int end_line) {
    namespace fs = std::filesystem;

    if (!fs::exists(path))
        return "File not found: " + path;

    std::string extension = fs::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
            return std::tolower(c); });

    if (std::find(TEXT_EXTENSIONS.begin(), TEXT_EXTENSIONS.end(), extension) != TEXT_EXTENSIONS.end())
        return read_text_file(path, start_line, end_line);
    else if (extension == ".csv")
        return read_csv_file(path);
    // TODO: Excel files
    // else if (extension == ".xlsx" || exention == ".xls")
        // return read_excel_file(path);
    else 
        return "We currently don't support this file type: " + extension;
}

std::string read_text_file(const std::string &path, int start_line, int end_line) {
    std::vector<std::string> lines;
    std::string line;

    std::ifstream file(path);

    if (!file.is_open())
        return "Could not open CSV file: " + path;

    while (std::getline(file, line))
        lines.push_back(line);

    if (start_line < 1 || (end_line != -1 && end_line < start_line)) 
        return "read_text_file: Start and end line indexes are incorrect";

    size_t start_idx = std::max(0, start_line - 1);
    size_t end_idx = end_line == -1 ? lines.size() : std::min(static_cast<size_t>(end_line), lines.size());

    std::vector<std::string> selected_lines(lines.begin() + start_idx, lines.begin() + end_idx);

    std::string result;
    size_t counter = start_line;
    for (const auto &line : selected_lines)
        result += std::to_string(counter++) + " | " + line + '\n';

    return result;
}

std::string read_csv_file(const std::string &path) {
    std::ifstream file(path);

    if (!file.is_open())
        return "Could not open CSV file: " + path;

    std::string line;
    std::string result; 

    while (std::getline(file, line)) {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;

        for (size_t i=0; i<line.size(); ++i) {
            char c = line[i];
            if (c == ',' && !in_quotes) {
                fields.push_back(field);
                field.clear();
            }
            else if (c == '"') {
                if (in_quotes && i+1 < line.size() && line[i+1] == '"') {
                    field += '"';
                    ++i;
                }
                else {
                    in_quotes = !in_quotes;
                }
            }
            else {
                field += c;
            }
        }

        fields.push_back(field);

        for (size_t i=0; i<fields.size(); ++i) {
            result += fields[i];
            
            if (i+1 < fields.size())
                result += " | ";
        }
        result += '\n';
    }

    return result;
}

// TODO: Requires dependencies, Come back too
/* Analyze an image, audio, or PDF file using an LLM */
void read_media_file_tool() {
    
}

// TODO: Requires dependencies, Come back too
/* Extract a zip file to the specified directory*/
void unzip_file_tool() {
    
}

int main(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::vector<std::string> documents = {
        {"The cat is sleeping on the couch"},
        {"A kitten is playing with a toy"},
        {"The dog is running in the park"},
        {"Python is a programming language"},
        {"Machine learning uses Python extensively"},
        {"Cats are popular pets"},
        {"Deep learning is a subset of machine learning"},
    };

    std::vector<std::vector<float>> doc_embeddings = get_embeddings(documents, embed_model);
    std::vector<std::pair<float, std::string>> results = vector_search("Artificial Intelligence",
                                                                       documents, 
                                                                       doc_embeddings,
                                                                       4);
    for (const auto &result : results) 
        std::cout << result.first << ": " << result.second << '\n';

    std::cout << read_csv_file("./test_csv.csv") << '\n';

    curl_global_cleanup();
    return 0;
}

// #include "agent.hpp"
// #include "llm_client.hpp"
// #include "web_search_tool.hpp"
//
// #include <iostream>
// #include <curl/curl.h>
//
// const std::string model = "qwen3.5:9b";
//
// int main() {
//     curl_global_init(CURL_GLOBAL_DEFAULT);
//
//     LlmClient client(model);
//     WebSearchTool search;
//     Agent agent(client, {&search});
//     AgentResult result = agent.run("What is the captial of Australia?");
//
//    if (result.output.has_value()) {
//         if (const auto *output = std::get_if<Json>(&*result.output))
//             std::cout << output->dump(4) << '\n';
//         else 
//             std::cout << std::get<std::string>(*result.output) << '\n';
//    }
//
//     curl_global_cleanup();
// }
