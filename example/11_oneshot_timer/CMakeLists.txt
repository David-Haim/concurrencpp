cmake_minimum_required(VERSION 3.16)

project(11_oneshot_time LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(11_oneshot_time source/main.cpp)

target_compile_features(11_oneshot_time PRIVATE cxx_std_20)

target_link_libraries(11_oneshot_time PRIVATE concurrencpp::concurrencpp)

target_coroutine_options(11_oneshot_time)
