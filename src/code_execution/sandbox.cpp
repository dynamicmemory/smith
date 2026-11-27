#include "sandbox.hpp"

#include "types.hpp"

#include <cerrno>
#include <csignal>
#include <stdexcept>
#include <string>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

const char *python_bootstrap = R"(
import sys
import json
import io
import contextlib
import traceback

namespace = {"__name__": "__main__"}

for line in sys.stdin:
    try:
        request = json.loads(line)

        stdout = io.StringIO()
        stderr = io.StringIO()
        error = ""

        try:
            with contextlib.redirect_stdout(stdout), \
                 contextlib.redirect_stderr(stderr):
                exec(
                    compile(request["code"], "<agent>", "exec"),
                    namespace
                )
        except BaseException:
            error = traceback.format_exc()

        response = {
            "stdout": stdout.getvalue(),
            "stderr": stderr.getvalue(),
            "error": error
        }

        sys.__stdout__.write(json.dumps(response) + "\n")
        sys.__stdout__.flush()

    except BaseException:
        response = {
            "stdout": "",
            "stderr": "",
            "error": traceback.format_exc()
        }

        sys.__stdout__.write(json.dumps(response) + "\n")
        sys.__stdout__.flush()
)";

bool write_all(int fd, const std::string &data) {
    size_t written = 0;

    while (written < data.size()) {
        ssize_t result = write(
            fd,
            data.data() + written,
            data.size() - written
        );

        if (result < 0) {
            if (errno == EINTR)
                continue;

            return false;
        }

        written += static_cast<size_t>(result);
    }

    return true;
}

std::string read_line(int fd) {
    std::string line;
    char character;

    while (true) {
        ssize_t result = read(fd, &character, 1);

        if (result < 0) {
            if (errno == EINTR)
                continue;

            throw std::runtime_error("Failed to read from sandbox");
        }

        if (result == 0)
            throw std::runtime_error("Sandbox process closed its output");

        if (character == '\n')
            break;

        line += character;
    }

    return line;
}

}

Sandbox::Sandbox() = default;

Sandbox::~Sandbox() {
    stop();
}

void Sandbox::start() {
    if (process_id != -1)
        throw std::runtime_error("Sandbox is already running");

    int input_pipe[2];
    int output_pipe[2];

    if (pipe(input_pipe) == -1)
        throw std::runtime_error("Failed to create input pipe");

    if (pipe(output_pipe) == -1) {
        close(input_pipe[0]);
        close(input_pipe[1]);
        throw std::runtime_error("Failed to create output pipe");
    }

    pid_t pid = fork();

    if (pid == -1) {
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);

        throw std::runtime_error("Failed to fork sandbox process");
    }

    if (pid == 0) {
        // Child process

        setpgid(0, 0);

        close(input_pipe[1]);
        close(output_pipe[0]);

        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);

        close(input_pipe[0]);
        close(output_pipe[1]);

        execlp(
            "bwrap",
            "bwrap",
            "--ro-bind", "/usr", "/usr",
            "--ro-bind", "/bin", "/bin",
            "--ro-bind", "/lib", "/lib",
            "--ro-bind", "/lib64", "/lib64",
            "--proc", "/proc",
            "--dev", "/dev",
            "--tmpfs", "/tmp",
            "--unshare-all",
            "--die-with-parent",
            "/usr/bin/python3",
            "-u",
            "-c",
            python_bootstrap,
            static_cast<char*>(nullptr)
        );

        _exit(127);
    }

    // Parent process

    close(input_pipe[0]);
    close(output_pipe[1]);

    input_fd = input_pipe[1];
    output_fd = output_pipe[0];
    process_id = static_cast<int>(pid);
}

SandboxResult Sandbox::execute(const std::string &code) {
    SandboxResult result;

    if (process_id == -1) {
        result.error = "Sandbox is not running";
        return result;
    }

    try {
        Json request = {
            {"code", code}
        };

        std::string message = request.dump() + "\n";

        if (!write_all(input_fd, message)) {
            result.error = "Failed to send code to sandbox";
            return result;
        }

        Json response = Json::parse(read_line(output_fd));

        result.stdout_output = response.value("stdout", "");
        result.stderr_output = response.value("stderr", "");
        result.error = response.value("error", "");
        result.success = result.error.empty();

    }
    catch (const std::exception &e) {
        result.error = e.what();
    }

    return result;
}

void Sandbox::stop() {
    if (process_id == -1)
        return;

    close(input_fd);
    close(output_fd);

    input_fd = -1;
    output_fd = -1;

    // Terminate the sandbox process group.
    kill(-process_id, SIGKILL);

    int status;
    while (waitpid(process_id, &status, 0) == -1 && errno == EINTR) {}

    process_id = -1;
}

