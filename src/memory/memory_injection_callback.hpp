#pragma once

#include "memory_manager.hpp"
#include "agent.hpp"

BeforeLlmCallback make_memory_injection_callback(MemoryManager &memory_manager);
