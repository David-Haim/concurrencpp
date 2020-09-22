cmake_minimum_required(VERSION 3.16)
project(executor_test)

add_executable(
	executor_test
	main.cpp
	)

target_link_libraries(executor_test concurrencpp)
