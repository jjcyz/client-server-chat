#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure and build
cmake ..
make server_test

# Run the tests with color output
./server_test --gtest_color=yes

# Store the exit code
exit_code=$?

# Return to original directory
cd ..

# Exit with the test exit code
exit $exit_code
