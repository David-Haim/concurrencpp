cmake_minimum_required(VERSION 3.16)

project(concurrencppTests LANGUAGES CXX)

include(../cmake/coroutineOptions.cmake)

# ---- Add root project ----

# Enable warnings from includes
set(concurrencpp_INCLUDE_WITHOUT_SYSTEM ON CACHE INTERNAL "")

# Enable thread sanitizer tests on Clang
if(CXX_COMPILER_ID MATCHES "Clang")
  set(CONCURRENCPP_ENABLE_THREAD_SANITIZER ON CACHE INTERNAL "")
endif()

if(TEST_INSTALLED_VERSION)
  find_package(concurrencpp REQUIRED CONFIG)
else()
  include(FetchContent)
  FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
  FetchContent_MakeAvailable(concurrencpp)
endif()

# ---- Test ----

enable_testing()

# TODO: remove this once tests were properly fixed
set(test_sources
        source/helpers/assertions.cpp
        source/helpers/object_observer.cpp
        source/main.cpp
        source/tester/tester.cpp
        source/tests/all_tests.cpp
        source/tests/coroutine_tests/coroutine_promises_test.cpp
        source/tests/coroutine_tests/coroutines_tests.cpp
        source/tests/executor_tests/inline_executor_tests.cpp
        source/tests/executor_tests/manual_executor_tests.cpp
        source/tests/executor_tests/thread_executor_tests.cpp
        source/tests/executor_tests/thread_pool_executor_tests.cpp
        source/tests/executor_tests/worker_thread_executor_tests.cpp
        source/tests/result_tests/make_result_tests.cpp
        source/tests/result_tests/result_await_tests.cpp
        source/tests/result_tests/result_promise_tests.cpp
        source/tests/result_tests/result_resolve_tests.cpp
        source/tests/result_tests/result_tests.cpp
        source/tests/result_tests/when_all_tests.cpp
        source/tests/result_tests/when_any_tests.cpp
        source/tests/runtime_tests.cpp
        source/tests/timer_tests/timer_queue_tests.cpp
        source/tests/timer_tests/timer_tests.cpp
        source/thread_sanitizer/executors.cpp
        source/thread_sanitizer/fibbonacci.cpp
        source/thread_sanitizer/matrix_multiplication.cpp
        source/thread_sanitizer/quick_sort.cpp
        source/thread_sanitizer/result.cpp
        source/thread_sanitizer/when_all.cpp
        source/thread_sanitizer/when_any.cpp)

set(test_headers
        include/helpers/assertions.h
        include/helpers/object_observer.h
        include/helpers/random.h
        include/helpers/result_generator.h
        include/helpers/thread_helpers.h
        include/tester/tester.h
        include/tests/all_tests.h
        include/tests/executor_tests/executor_test_helpers.h
        include/tests/test_utils/executor_shutdowner.h
        include/tests/test_utils/proxy_coro.h
        include/tests/test_utils/result_factory.h
        include/tests/test_utils/test_executors.h
        include/tests/test_utils/test_ready_result.h)

add_executable(dummy_to_remove ${test_sources} ${test_headers})
target_link_libraries(dummy_to_remove PRIVATE concurrencpp::concurrencpp)
target_compile_features(dummy_to_remove PRIVATE cxx_std_20)
target_coroutine_options(dummy_to_remove)
target_include_directories(dummy_to_remove
        PRIVATE
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
add_test(NAME dummy_to_remove COMMAND dummy_to_remove)

return()

# add_test(NAMESPACE <namespace> NAME <name> [PROPERTIES ...])
#
# Add test with the name <namespace>.<name> with the source file at
# source/<namespace>/<name>.cpp with project specific options that outputs the
# exectuable <namespace>_<name>.
#
# Additional properties may be forwarded to `set_tests_properties` using the
# PROPERTIES arguments.
# See: https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-tests
#
function(add_test)
  cmake_parse_arguments(TEST "" "NAMESPACE;NAME" "PROPERTIES" ${ARGN})

  set(target "${TEST_NAMESPACE}_${TEST_NAME}")
  set(test_name "${TEST_NAMESPACE}.${TEST_NAME}")

  add_executable(${target} "source/${TEST_NAMESPACE}/${TEST_NAME}.cpp")

  target_link_libraries(${target} PRIVATE concurrencpp::concurrencpp)

  target_compile_features(${target} PRIVATE cxx_std_20)

  target_coroutine_options(${target})

  target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/include")

  # Call the original add_test
  _add_test(NAME ${test_name} COMMAND ${target})

  if(TEST_PROPERTIES)
    set_tests_properties(${test_name} PROPERTIES ${TEST_PROPERTIES})
  endif()
endfunction()

# TODO: add tests

if(NOT CONCURRENCPP_ENABLE_THREAD_SANITIZER)
  return()
endif()

# TODO: add thread sanitizer tests
