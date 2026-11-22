#pragma once 

#include "session_manager.hpp"
#include <unordered_map>

class SQLiteSessionManager : public BaseSessionManager {
public:
    std::shared_ptr<Session> create(
            const std::string &session_id,
            std::optional<std::string> user_id = std::nullopt) override;

    std::shared_ptr<Session> get(const std::string &session_id) override;

    void save(const Session &session) override;

private:
    std::unordered_map<std::string, std::shared_ptr<Session>> session;
};
