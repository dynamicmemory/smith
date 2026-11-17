cmake_minimum_required(VERSION 3.20)
project(smith VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(smith
    src/main.cpp

    src/llm/llm_client.cpp
    src/llm/ollama.cpp

    src/agent/agent.cpp
    src/agent/agents.cpp

    src/rag/rag.cpp

    src/tools/web_search/web_search_tool.cpp
    src/tools/file_system/list_files_tool.cpp
    src/tools/file_system/read_file_tool.cpp
)

target_include_directories(smith PRIVATE 
    src
    src/core
    src/agent
    src/tools
    src/tools/file_system
    src/tools/web_search
    src/rag
    src/llm
)

target_link_libraries(smith PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
target_compile_options(smith PRIVATE -Wall -Wextra -Wpedantic)
