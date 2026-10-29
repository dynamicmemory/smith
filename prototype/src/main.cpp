
#include "smith/agent.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

int main(int argc, char* argv[]) {
    std::string model_name = "qwen3.5:9b";
    std::filesystem::path project_root = std::filesystem::current_path();

    if (argc == 2) {
        std::filesystem::path argument(argv[1]);

        if (std::filesystem::is_directory(argument)) {
            project_root = argument;
        } else {
            model_name = argv[1];
        }
    } else if (argc == 3) {
        if (std::string(argv[1]) == "--model") {
            model_name = argv[2];
        } else {
            std::cerr << "Usage:\n"
                      << "  smith\n"
                      << "  smith <project-directory>\n"
                      << "  smith --model <model-name> [project-directory]\n";
            return 1;
        }
    } else if (argc == 4) {
        if (std::string(argv[1]) != "--model") {
            std::cerr << "Usage:\n"
                      << "  smith --model <model-name> <project-directory>\n";
            return 1;
        }

        model_name = argv[2];
        project_root = argv[3];
    } else if (argc > 4) {
        std::cerr << "Too many arguments.\n";
        return 1;
    }

    try {
        project_root = std::filesystem::canonical(project_root);

        if (!std::filesystem::is_directory(project_root)) {
            std::cerr << "Project path is not a directory.\n";
            return 1;
        }

        ModelClient model(model_name);

        Agent agent(
            std::move(model),
            "You are Smith, a concise coding assistant and development agent. "
            "Answer the user's latest message directly and follow requested formats. "
            "Use available tools when inspecting the project would help answer or complete a task. "
            "Tool paths are relative to the project root. Do not claim to have read files unless you used a tool. "
            "For source-code inspection, list each needed directory at most once. "
            "When several independent files need reading, use read_files with their paths in one call rather than narrating each step or issuing one read_file call at a time. "
            "Do not narrate plans or progress. If you need to inspect files, call the tools directly. "
            "Never repeat a tool call with the same name and arguments. "
            "For requests about each source file, inspect the source files systematically and summarize each inspected file's actual purpose. "
            "Once you have enough evidence to answer, stop using tools and give the final answer.",
            project_root
        );

        std::cout << "Smith project: " << project_root << '\n';
        std::cout << "Model: " << model_name << '\n';

        std::string input;
        while (true) {
            std::cout << "\nYou> ";
            if (!std::getline(std::cin, input) || input == "/quit")
                break;

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

