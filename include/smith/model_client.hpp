#pragma once 

#include <string>
#include <string_view>

class ModelClient {
public:
    explicit ModelClient(std::string model, std::string base_url = "http://127.0.0.1:11434");

    std::string chat(std::string_view message) const;

private:
    std::string model_;
    std::string endpoint_;
};


