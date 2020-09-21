cmake_minimum_required(VERSION 3.16)
project(async_file_processing)

add_executable(
	async_file_processing
	src/main.cpp
)

target_link_libraries(async_file_processing concurrencpp)
