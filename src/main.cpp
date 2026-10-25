// TODO: Implement the model client, tool execution, and agent loop as sep components
#include "smith/model_client.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: smith <model> <message>\n";
        return 1;
    }

    try {
        ModelClient model(argv[1]);

        const std::string response = model.chat(argv[2]);
        std::cout << response << '\n';
    }
    catch (const std::exception &error) {
        std::cerr << "Smith: " << error.what() << '\n';
        return 1;
    }
    return 0;
}


