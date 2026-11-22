#pragma once 

#include "session.hpp"
#include <memory>
#include <optional>
#include <string>

class BaseSessionManager {
public:
    virtual ~BaseSessionManager() = default;

    virtual std::shared_ptr<Session> create(
            const std::string &session_id,
            std::optional<std::string> user_id = std::nullopt) = 0;

    virtual std::shared_ptr<Session> get(const std::string &session_id) = 0;

    virtual void save(const Session &session) = 0;

    std::shared_ptr<Session> get_or_create( 
            const std::string &session_id,
            std::optional<std::string> user_id = std::nullopt);
};
