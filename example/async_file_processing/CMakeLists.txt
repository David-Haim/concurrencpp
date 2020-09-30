cmake_minimum_required(VERSION 3.16)

project(async_file_processing LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(async_file_processing source/main.cpp)

target_compile_features(async_file_processing PRIVATE cxx_std_20)

target_link_libraries(async_file_processing PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(async_file_processing)
