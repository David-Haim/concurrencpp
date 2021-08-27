cmake_minimum_required(VERSION 3.16)

project(9_result_promise LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(9_result_promise source/main.cpp)

target_compile_features(9_result_promise PRIVATE cxx_std_20)

target_link_libraries(9_result_promise PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(9_result_promise)
