cmake_minimum_required(VERSION 3.14)
project(chat_server)

# Google Test requires at least C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${SQLite3_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

# Add source files (excluding main.cpp)
file(GLOB SERVER_SOURCES "src/*.cpp")
list(REMOVE_ITEM SERVER_SOURCES "${PROJECT_SOURCE_DIR}/src/main.cpp")

# Create library from source files
add_library(server_lib ${SERVER_SOURCES})

# Create the main server executable
add_executable(server src/main.cpp)
target_link_libraries(server server_lib pthread ${SQLite3_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARY})

# Create the client executable
add_executable(client src/client.cpp)
target_link_libraries(client pthread)

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE  # Added this line to fix the warning
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Add the tests
add_executable(server_test tests/server_test.cpp)
target_link_libraries(server_test
    server_lib
    GTest::gtest_main
    pthread
)

add_executable(client_test tests/client_test.cpp)
target_link_libraries(client_test
    GTest::gtest_main
    pthread
)

include(GoogleTest)
gtest_discover_tests(server_test)
gtest_discover_tests(client_test)

# Add custom targets for testing
add_custom_target(build_tests
    DEPENDS server_test
    DEPENDS client_test
)

add_custom_target(run_all_tests
    COMMAND ./server_test --gtest_color=yes
    COMMAND ./client_test --gtest_color=yes
    DEPENDS server_test
    DEPENDS client_test
)
