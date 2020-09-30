cmake_minimum_required(VERSION 3.16)

project(sandbox LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(concurrencpp)

include(../cmake/coroutineOptions.cmake)

add_executable(sandbox main.cpp)

target_compile_features(sandbox PRIVATE cxx_std_20)

target_link_libraries(sandbox PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(sandbox)
