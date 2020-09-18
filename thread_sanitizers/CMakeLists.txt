cmake_minimum_required(VERSION 3.0)
project(thread_sanitizers)

SET(CMAKE_CXX_STANDARD 17)

add_subdirectory(executors)
add_subdirectory(result)
add_subdirectory(fibbonacci)
add_subdirectory(quick_sort)
add_subdirectory(matrix_multiplication)
add_subdirectory(when_all)
add_subdirectory(when_any)