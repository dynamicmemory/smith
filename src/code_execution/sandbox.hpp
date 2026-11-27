#pragma once 

#include <string>

struct SandboxResult {
    std::string stdout_output;
    std::string stderr_output;
    std::string error;
    bool success = false;
};

class Sandbox {
public:
    Sandbox();
    ~Sandbox();

    Sandbox(const Sandbox&) = delete;
    Sandbox &operator=(const Sandbox &) = delete;

    void start();
    SandboxResult execute(const std::string &code);
    void stop();

private:
    int input_fd = -1;
    int output_fd = -1;
    int process_id = -1;
};
