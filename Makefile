CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1
LDFLAGS = -pthread

SRCS = main.cpp network_handler.cpp connection_pool.cpp message_queue.cpp server_metrics.cpp command_processor.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
