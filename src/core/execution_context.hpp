#pragma once 

#include "types.hpp"
#include "session.hpp"
#include "session_manager.hpp"
#include <string>
#include <vector>
#include <map>
#include <utility>
#include <optional>
#include <memory>
#include <chrono>

struct ExecutionContext {
    std::string execution_id; // uuid?
    int current_step = 0;

    std::shared_ptr<Session> session;
    BaseSessionManager *session_manager = nullptr;

    std::vector<Event> &events() { return session->events; }
    std::map<std::string, Json> &state() { return session->state; }

    const std::vector<Event> &events() const { return session->events; }
    const std::map<std::string, Json> &state() const { return session->state; }

    std::optional<Json> final_result;

    std::optional<std::string> code_env = std::nullopt;

    void add_event(Event event) { 
        events().push_back(std::move(event)); 
        session->updated_at = std::chrono::system_clock::now();
    }

    void increment_step() { current_step++; }
};
