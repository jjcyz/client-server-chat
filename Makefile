CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I./include -I/Library/Developer/CommandLineTools/usr/include/c++/v1
LDFLAGS = -pthread -lssl -lcrypto -lsqlite3
# Google Test configuration
GTEST_DIR = tests/lib/googletest/googletest
GTEST_INCLUDE = -I$(GTEST_DIR)/include
GTEST_LIB = $(GTEST_DIR)/lib/libgtest.a
GTEST_MAIN_LIB = $(GTEST_DIR)/lib/libgtest_main.a

# Source files (excluding main.cpp)
SERVER_SRCS = src/network_handler.cpp \
              src/connection_pool.cpp \
              src/message_queue.cpp \
              src/server_metrics.cpp \
              src/command_processor.cpp \
              src/socket_utils.cpp \
              src/server.cpp \
              src/database.cpp

# Main source file
MAIN_SRC = src/main.cpp

# Object files
SERVER_OBJS = $(patsubst src/%.cpp,build/%.o,$(SERVER_SRCS))
MAIN_OBJ = build/main.o

# Targets
SERVER_TARGET = server
TEST_TARGET = build/server_test
CLIENT_TARGET = client

.PHONY: all clean test

all: directories $(SERVER_TARGET) $(CLIENT_TARGET)

directories:
	@mkdir -p build
	@mkdir -p build/tests

# Build server library (without main)
build/libserver.a: $(SERVER_OBJS)
	ar rcs $@ $^

$(SERVER_TARGET): $(MAIN_OBJ) build/libserver.a
	$(CXX) -o $@ $^ $(LDFLAGS)

$(CLIENT_TARGET): src/client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

build/%.o: src/%.cpp | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Google Test rules
$(GTEST_LIB):
	cd $(GTEST_DIR) && cmake . && make

# Build and run tests
$(TEST_TARGET): tests/server_test.cpp build/libserver.a $(GTEST_LIB)
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE) $^ -o $@ $(GTEST_LIB) $(GTEST_MAIN_LIB) $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET) --gtest_color=yes

clean:
	rm -rf build $(SERVER_TARGET) $(CLIENT_TARGET)
