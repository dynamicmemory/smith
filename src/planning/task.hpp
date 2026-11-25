#pragma once 

#include <string>

enum class TaskStatus {
    pending,
    in_progress,
    completed
};

struct Task {
    std::string content;
    TaskStatus status = TaskStatus::pending;
};

