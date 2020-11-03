cmake_minimum_required(VERSION 3.16)

project(async_sql LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
FetchContent_MakeAvailable(concurrencpp)

include(../../cmake/coroutineOptions.cmake)

add_executable(async_sql
        include/mock_async_sql.h
        source/main.cpp
        source/mock_async_sql.cpp)

target_compile_features(async_sql PRIVATE cxx_std_20)

target_link_libraries(async_sql PRIVATE concurrencpp::concurrencpp)

target_include_directories(async_sql PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_coroutine_options(async_sql)
