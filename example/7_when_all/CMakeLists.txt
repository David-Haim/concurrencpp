cmake_minimum_required(VERSION 3.16)

project(7_when_all LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(7_when_all source/main.cpp)

target_compile_features(7_when_all PRIVATE cxx_std_20)

target_link_libraries(7_when_all PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(7_when_all)
