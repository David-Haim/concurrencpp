cmake_minimum_required(VERSION 3.16)
project(matrix_multiplication_test)

add_executable(
	matrix_multiplication_test
	main.cpp
	)

target_link_libraries(matrix_multiplication_test concurrencpp)
