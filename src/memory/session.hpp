#pragma once

#include "types.hpp"
#include <string>
#include <optional>
#include <vector>
#include <map>
#include <chrono>

struct Session {
    std::string session_id;
    std::optional<std::string> user_id;
    std::vector<Event> events;
    std::map<std::string, Json> state;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};
