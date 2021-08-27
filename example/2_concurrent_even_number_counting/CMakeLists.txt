cmake_minimum_required(VERSION 3.16)

project(2_concurrent_even_number_counting LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(2_concurrent_even_number_counting source/main.cpp)

target_compile_features(2_concurrent_even_number_counting PRIVATE cxx_std_20)

target_link_libraries(2_concurrent_even_number_counting PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(2_concurrent_even_number_counting)
