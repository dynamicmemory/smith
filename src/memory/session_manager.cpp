#include "session_manager.hpp"

std::shared_ptr<Session> BaseSessionManager::get_or_create(
        const std::string &session_id,
        std::optional<std::string> user_id) {

    auto session = get(session_id);

    if (!session)
        session = create(session_id, user_id);

    return session;
}
