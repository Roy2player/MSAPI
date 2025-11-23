cmake_minimum_required(VERSION 3.2)

project(TestHttp VERSION 1.0 LANGUAGES CXX)

set(SOURCE
        ../source/main.cpp
        ../source/httpClient.cpp
        ../source/httpServer.cpp
)

include(../../../../library/build/CMakeListsCommonOptions.txt)

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/build/libMSAPI.so")

msapi_set_target_properties(${PROJECT_NAME})