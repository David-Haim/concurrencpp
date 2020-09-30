cmake_minimum_required(VERSION 3.16)

project(synchronous_web_socket LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(synchronous_web_socket
        include/mock_web_socket.h
        source/main.cpp
        source/mock_web_socket.cpp)

target_compile_features(synchronous_web_socket PRIVATE cxx_std_20)

target_link_libraries(synchronous_web_socket PRIVATE concurrencpp::concurrencpp)

target_include_directories(synchronous_web_socket PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_coroutine_options(synchronous_web_socket)
