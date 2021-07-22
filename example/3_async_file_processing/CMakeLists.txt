cmake_minimum_required(VERSION 3.16)

project(3_async_file_processing LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(3_async_file_processing source/main.cpp)

target_compile_features(3_async_file_processing PRIVATE cxx_std_20)

target_link_libraries(3_async_file_processing PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(3_async_file_processing)
