#pragma once 

#include <functional>
#include <string>
#include <vector>

struct ChatMessage {
    std::string role;
    std::string content;
};

class ModelClient {
public:
    using StreamCallback = std::function<void(const std::string &)>;

    explicit ModelClient(std::string model, 
                         std::string base_url = "http://127.0.0.1:11434");

    std::string chat(const std::vector<ChatMessage> &messages, 
                     StreamCallback on_chunk) const;

private:
    std::string model_;
    std::string endpoint_;
};


