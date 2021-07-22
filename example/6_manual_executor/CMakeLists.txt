cmake_minimum_required(VERSION 3.16)

project(6_manual_executor LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(6_manual_executor source/main.cpp)

target_compile_features(6_manual_executor PRIVATE cxx_std_20)

target_link_libraries(6_manual_executor PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(6_manual_executor)
