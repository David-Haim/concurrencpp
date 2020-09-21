cmake_minimum_required(VERSION 3.17.0)
project(process_monitor)

add_executable(
	process_monitor
	src/main.cpp
	src/mock_process_monitor.cpp
)

target_link_libraries(process_monitor concurrencpp)
