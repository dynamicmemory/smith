#pragma once 

#include "types.hpp"
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <optional>

struct ExecutionContext {
    std::string execution_id; // uuid?
    std::vector<Event> events;
    int current_step = 0;
    std::map<std::string, Json> state;
    std::optional<Json> final_result;

    void add_event(Event event) { events.push_back(std::move(event)); }
    void increment_step(void) { current_step++; }
};
