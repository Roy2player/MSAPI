cmake_minimum_required(VERSION 3.13)

project(UnitTestWebSocketEvents VERSION 1.0 LANGUAGES CXX)

set(SOURCE
        ../source/main.cpp
        ../source/events.cpp
        ../source/initialization.cpp
        ../source/removalAndLimits.cpp
        ../source/deferredPurging.cpp
        ../source/termination.cpp
        ../source/transport.cpp
        ../source/delayedTransport.cpp
        ../source/buffersAndRegistration.cpp
        ../source/delayedDelivery.cpp
        ../source/delayedOwnership.cpp
)

include(../../../../library/build/CMakeListsCommonOptions.txt)

add_executable(${PROJECT_NAME} ${SOURCE})

target_link_libraries(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/../../../../library/build/libMSAPI.so")

msapi_set_target_properties(${PROJECT_NAME})