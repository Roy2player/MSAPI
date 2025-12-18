cmake_minimum_required(VERSION 3.2)

project(MSAPI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin
    CACHE PATH
    "Single Directory for all"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin
    CACHE PATH
    "Single Directory for all"
)

set(SOURCE_CPP 
        ../source/help/helper.cpp
        ../source/help/html.cpp
        ../source/help/identifier.cpp
        ../source/help/json.cpp
        ../source/help/log.cpp
        ../source/help/time.cpp
        ../source/help/diagnostic.cpp
        ../source/help/table.cpp
        ../source/protocol/http.cpp
        ../source/protocol/dataHeader.cpp
        ../source/protocol/object.cpp
        ../source/protocol/standard.cpp
        ../source/server/application.cpp
        ../source/server/server.cpp
        ../source/test/test.cpp
        ../source/test/actionsCounter.cpp
)

include(./CMakeListsCommonOptions.txt)

add_library(${PROJECT_NAME} SHARED ${SOURCE_CPP})

msapi_set_target_properties(${PROJECT_NAME})