#include "smith/agent.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <utility>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: smith <model>\n";
        return 1;
    }

    try {
        ModelClient model(argv[1]);
        Agent agent(std::move(model),
            "You are Smith, a concise coding assistant and development agent. "
            "Answer the user's latest message directly and follow requested formats. "
            "Use available tools when inspecting the project would help answer or complete a task. "
            "Tool paths are relative to the project root. Do not claim to have read files unless you used a tool. "
            "For source-code inspection, list each needed directory at most once. "
            "When several independent files need reading, use read_files with their paths in one call rather than narrating each step or issuing one read_file call at a time. "
            "Do not narrate plans or progress. If you need to inspect files, call the tools directly. "
            "Never repeat a tool call with the same name and arguments. "
            "For requests about each source file, inspect the source files systematically and summarize each inspected file's actual purpose. "
            "Once you have enough evidence to answer, stop using tools and give the final answer.");

        std::string input;
        while (true) {
            std::cout << "\nYou> ";
            if (!std::getline(std::cin, input) || input == "/quit") break;
            std::cout << "\nSmith> " << std::flush;
            try {
                agent.respond(input, [](const std::string& chunk) {
                    std::cout << chunk << std::flush;
                });
                std::cout << '\n';
            } catch (const std::exception& error) {
                std::cerr << "\nRequest failed: " << error.what() << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Smith: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
