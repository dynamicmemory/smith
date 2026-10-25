#include "ollama_response.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void check(
    std::string_view name,
    std::string_view expected,
    std::string_view actual
) {
    if (expected != actual) {
        std::cerr << "FAIL: " << name
                  << "\n  Expected: " << expected
                  << "\n  Actual:   " << actual << '\n';
        ++failures;
    } else {
        std::cout << "PASS: " << name << '\n';
    }
}

void check_throws(
    std::string_view name,
    std::string_view body
) {
    try {
        (void)parse_ollama_response(body);
        std::cerr << "FAIL: " << name
                  << " (expected an exception)\n";
        ++failures;
    } catch (const std::exception&) {
        std::cout << "PASS: " << name << '\n';
    }
}

} // namespace

int main() {
    check(
        "valid response",
        "Hello!",
        parse_ollama_response(
            R"({"message":{"role":"assistant","content":"Hello!"},"done":true})"
        )
    );

    check(
        "escaped characters",
        "Hello\n\"Smith\"",
        parse_ollama_response(
            R"({"message":{"content":"Hello\n\"Smith\""}})"
        )
    );

    check_throws(
        "missing message",
        R"({"done":true})"
    );

    check_throws(
        "missing content",
        R"({"message":{"role":"assistant"}})"
    );

    check_throws(
        "non-string content",
        R"({"message":{"content":42}})"
    );

    check_throws(
        "malformed JSON",
        R"({"message":)"
    );

    if (failures != 0) {
        std::cerr << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "All tests passed.\n";
    return 0;
}
