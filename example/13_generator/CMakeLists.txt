cmake_minimum_required(VERSION 3.16)

project(13_generator LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(13_generator source/main.cpp)

target_compile_features(13_generator PRIVATE cxx_std_20)

target_link_libraries(13_generator PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(13_generator)
