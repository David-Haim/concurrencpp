cmake_minimum_required(VERSION 3.16)

project(12_delay_object LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(12_delay_object source/main.cpp)

target_compile_features(12_delay_object PRIVATE cxx_std_20)

target_link_libraries(12_delay_object PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(12_delay_object)
