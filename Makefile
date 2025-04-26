CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I/Library/Developer/CommandLineTools/usr/include/c++/v1
LDFLAGS = -pthread

SERVER_SRCS = src/main.cpp \
              src/network_handler.cpp \
              src/connection_pool.cpp \
              src/message_queue.cpp \
              src/server_metrics.cpp \
              src/command_processor.cpp \
              src/socket_utils.cpp \
              src/server.cpp

SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
SERVER_TARGET = server

CLIENT_SRCS = src/client.cpp
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)
CLIENT_TARGET = client

.PHONY: all clean

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET)
