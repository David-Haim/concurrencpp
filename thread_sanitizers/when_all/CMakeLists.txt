cmake_minimum_required(VERSION 3.17.0)
project(when_all_test)

add_executable(
	when_all_test
	main.cpp
	)

target_link_libraries(when_all_test concurrencpp)