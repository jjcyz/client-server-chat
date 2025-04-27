cmake_minimum_required(VERSION 3.14)
project(chat_server)

# Google Test requires at least C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add source files (excluding main.cpp)
file(GLOB SERVER_SOURCES "src/*.cpp")
list(REMOVE_ITEM SERVER_SOURCES "${PROJECT_SOURCE_DIR}/src/main.cpp")

# Create library from source files
add_library(server_lib ${SERVER_SOURCES})

# Create the main server executable
add_executable(server src/main.cpp)
target_link_libraries(server server_lib pthread)

# Create the client executable
add_executable(client src/client.cpp)
target_link_libraries(client pthread)

# Add Google Test
add_subdirectory(tests/lib/googletest)

# Enable testing
enable_testing()

# Add the tests
add_executable(server_test tests/server_test.cpp)
target_link_libraries(server_test
    server_lib
    gtest
    gtest_main
    pthread
)

# Add test to CTest
add_test(NAME server_test COMMAND server_test)

# Add custom test target with output on failure
add_custom_target(check
    COMMAND ctest --output-on-failure
    DEPENDS server_test)
