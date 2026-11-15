#pragma once 

#include "types.hpp"
#include <string>

Json ollama_post(const std::string &url, const Json &payload);
