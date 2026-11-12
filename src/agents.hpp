#pragma once 

#include "agent.hpp"

Agent create_gaia_agent(LlmClient &client, std::vector<BaseTool*> tools);
