cmake_minimum_required(VERSION 3.16)

project(10_regular_timer LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(10_regular_timer source/main.cpp)

target_compile_features(10_regular_timer PRIVATE cxx_std_20)

target_link_libraries(10_regular_timer PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(10_regular_timer)
