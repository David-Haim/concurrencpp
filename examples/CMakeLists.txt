cmake_minimum_required(VERSION 3.0)
project(examples)

SET(CMAKE_CXX_STANDARD 17)

add_subdirectory(async_file_processing)
add_subdirectory(async_sql)
add_subdirectory(process_monitor)
add_subdirectory(synchronous_web_socket)
