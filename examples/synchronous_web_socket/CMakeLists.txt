cmake_minimum_required(VERSION 3.16)
project(synchronous_web_socket)

add_executable(
	synchronous_web_socket
	src/main.cpp
	src/mock_web_socket.cpp
)

target_link_libraries(synchronous_web_socket concurrencpp)
