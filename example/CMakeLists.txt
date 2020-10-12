cmake_minimum_required(VERSION 3.16)

project(concurrencppExamples LANGUAGES CXX)

foreach(example IN ITEMS
        async_file_processing
        async_sql
        process_monitor
        synchronous_web_socket)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${example}"
          "${CMAKE_CURRENT_BINARY_DIR}/${example}")
endforeach()
