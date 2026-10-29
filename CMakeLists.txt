cmake_minimum_required(VERSION 3.20)
project(smith VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(smith
    src/main.cpp
)
target_include_directories(smith PRIVATE include src)
target_link_libraries(smith PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
target_compile_options(smith PRIVATE -Wall -Wextra -Wpedantic)

# if(BUILD_TESTING)
#     add_executable(smith_tests tests/model_client_test.cpp src/ollama_response.cpp)
    # target_include_directories(smith_tests PRIVATE src)
    # target_link_libraries(smith_tests PRIVATE nlohmann_json::nlohmann_json)
    # target_compile_options(smith_tests PRIVATE -Wall -Wextra -Wpedantic)
    # add_test(NAME smith_tests COMMAND smith_tests)
# endif()
