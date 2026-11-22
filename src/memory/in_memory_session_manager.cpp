#include "in_memory_session_manager.hpp"
#include <stdexcept>

std::shared_ptr<Session> InMemorySessionManager::create(
        const std::string &session_id,
        std::optional<std::string> user_id) {

    if (session.contains(session_id))
        throw std::runtime_error("Session already exists: " + session_id);

    auto new_session = std::make_shared<Session>();
    new_session->session_id = session_id;
    new_session->user_id = user_id;
    new_session->created_at = std::chrono::system_clock::now();
    new_session->updated_at = new_session->created_at;

    session[session_id] = new_session;
    return new_session;
}

std::shared_ptr<Session> InMemorySessionManager::get(const std::string &session_id) {
    auto it = session.find(session_id);

    if (it == session.end())
        return nullptr;

    return it->second;
}

void InMemorySessionManager::save(const Session &session_data) {
    session[session_data.session_id] = std::make_shared<Session>(session_data);
}
