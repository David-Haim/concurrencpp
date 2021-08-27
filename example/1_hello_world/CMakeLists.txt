cmake_minimum_required(VERSION 3.16)

project(1_hello_world LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(1_hello_world source/main.cpp)

target_compile_features(1_hello_world PRIVATE cxx_std_20)

target_link_libraries(1_hello_world PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(1_hello_world)
