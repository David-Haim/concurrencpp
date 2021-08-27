cmake_minimum_required(VERSION 3.16)

project(4_async_file_processing_version_2 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(4_async_file_processing_version_2 source/main.cpp)

target_compile_features(4_async_file_processing_version_2 PRIVATE cxx_std_20)

target_link_libraries(4_async_file_processing_version_2 PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(4_async_file_processing_version_2)
