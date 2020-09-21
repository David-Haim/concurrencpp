cmake_minimum_required(VERSION 3.17.0)
project(async_sql)

add_executable(
	async_sql
	src/main.cpp
	src/mock_async_sql.cpp
)

target_link_libraries(async_sql concurrencpp)
