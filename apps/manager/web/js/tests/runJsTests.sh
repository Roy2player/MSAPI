#!/bin/bash
#Run all the NodeJS tests from some directory which match the pattern *.test.js

#Check the number of arguments
if [ "$#" -gt 1 ]; then
    echo "Error: Too many arguments."
    echo "Usage: bash $0 [test_directory]"
    exit 1
fi

#Directory containing test files
if [ "$#" -eq 1 ]; then

    #Check if the directory exists
    if [ ! -d "$1" ]; then
        echo "Error: Directory '$1' does not exist."
        exit 1
    fi

    TEST_DIR="$1"
else
    TEST_DIR="."
fi

#Pattern to match test files
PATTERN="*.test.js"

#Find and run each test file
for test_file in "$TEST_DIR"/$PATTERN; do
    echo "Running $test_file"
    node "$test_file"

    if [ $? != 0 ]; then
        echo "Error: Test failed in $test_file"
        exit 1
    fi
done

echo "Finished running all tests"