#include "smith/model_client.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: smith <model>\n";
        return 1;
    }

    try {
        ModelClient model(argv[1]);

        std::vector<ChatMessage> history = {
            {"system", "You are Smith, a concise coding assistant."}
        };

        std::string input;

        while (true) {
            std::cout << "\nYou> ";

            if (!std::getline(std::cin, input) || input == "/quit") {
                break;
            }

            history.push_back({"user", input});

            std::cout << "\nSmith> " << std::flush;

            const std::string response = model.chat(
                history,
                [](const std::string& chunk) {
                    std::cout << chunk << std::flush;
                }
            );

            history.push_back({"assistant", response});

            std::cout << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << "\nSmith: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
