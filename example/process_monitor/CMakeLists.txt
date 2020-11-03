cmake_minimum_required(VERSION 3.16)

project(process_monitor LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(process_monitor
        include/mock_process_monitor.h
        source/main.cpp
        source/mock_process_monitor.cpp)

target_compile_features(process_monitor PRIVATE cxx_std_20)

target_link_libraries(process_monitor PRIVATE concurrencpp::concurrencpp)

target_include_directories(process_monitor PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_coroutine_options(process_monitor)
