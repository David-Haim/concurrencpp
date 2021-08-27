cmake_minimum_required(VERSION 3.16)

project(5_prime_number_finder LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(5_prime_number_finder source/main.cpp)

target_compile_features(5_prime_number_finder PRIVATE cxx_std_20)

target_link_libraries(5_prime_number_finder PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(5_prime_number_finder)
