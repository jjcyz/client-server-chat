CXX = g++
CXXFLAGS = -std=c++17 -pthread -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1
TARGETS = server stress_test

all: $(TARGETS)

server: server.cpp
	$(CXX) $(CXXFLAGS) server.cpp -o server

stress_test: stress_test.cpp
	$(CXX) $(CXXFLAGS) stress_test.cpp -o stress_test

clean:
	rm -f $(TARGETS)

.PHONY: all clean
