cmake_minimum_required(VERSION 3.16)
project(fibbonacci_test)

add_executable(
	fibbonacci_test
	main.cpp
	)

target_link_libraries(fibbonacci_test concurrencpp)
