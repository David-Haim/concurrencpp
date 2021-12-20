cmake_minimum_required(VERSION 3.16)

project(concurrencppExamples LANGUAGES CXX)

foreach(example IN ITEMS
	1_hello_world
	2_concurrent_even_number_counting
	3_async_file_processing
    4_async_file_processing_version_2
	5_prime_number_finder
    6_manual_executor
	7_when_all
    8_when_any
	9_result_promise
	10_regular_timer
    11_oneshot_timer
	12_delay_object
    13_generator
    )
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${example}"
          "${CMAKE_CURRENT_BINARY_DIR}/${example}")
endforeach()
