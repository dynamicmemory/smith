#include "ollama.hpp"
#include <iostream> 

std::string model = "qwen3.5:9b";

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // json response = chat(model, {{"user", "What is the colour red?"}}); 
    // std::cout << response << '\n';

    curl_global_cleanup();
    return 0;
}



