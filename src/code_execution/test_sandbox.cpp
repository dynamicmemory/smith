#include "sandbox.hpp"
#include <iostream>

int main() {
    Sandbox sandbox;

    sandbox.start();

    auto first = sandbox.execute("data = [1,2,3]");
    std::cout << "First execution: " << 
        (first.success ? "SUCCESS" : first.error) << '\n';

    auto second = sandbox.execute("print(sum(data))");
    std::cout << "Second execution: " << second.stdout_output;

    sandbox.stop();
}
