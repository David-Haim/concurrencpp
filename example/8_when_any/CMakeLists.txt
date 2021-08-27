cmake_minimum_required(VERSION 3.16)

project(8_when_any LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(8_when_any source/main.cpp)

target_compile_features(8_when_any PRIVATE cxx_std_20)

target_link_libraries(8_when_any PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(8_when_any)
