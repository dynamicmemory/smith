
#include "gaia.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <curl/curl.h>

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "CURL initialisation failed\n";
        return 1;
    }

    int exit_code = 0;

    try {
        std::vector<std::string> models = {
            "qwen3.5:9b"
        };

        std::vector<GaiaProblem> problems =
            load_gaia_dataset("data/GAIA/level1.jsonl");

        // Runs all 20 questions.
        auto results = run_experiment(problems, models);

        // Prints full results and accuracy after completion.
        print_results(results);
    }
    catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << '\n';
        exit_code = 1;
    }

    curl_global_cleanup();
    return exit_code;
}
