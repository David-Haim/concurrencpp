cmake_minimum_required(VERSION 3.16)

project(concurrencppTests LANGUAGES CXX)

include(../cmake/coroutineOptions.cmake)

# ---- Add root project ----

option(ENABLE_THREAD_SANITIZER "\
Build concurrencpp with LLVM thread sanitizer. \
Does not have an effect if the compiler is not Clang based." OFF)

string(TOLOWER ${CMAKE_CXX_COMPILER_ID} compiler_id)
if(ENABLE_THREAD_SANITIZER AND compiler_id MATCHES "clang")
  # Instead of polluting the lists file, we inject a command definition
  # override that will apply the sanitizer flag
  set(CMAKE_PROJECT_concurrencpp_INCLUDE
          "${CMAKE_CURRENT_LIST_DIR}/../cmake/concurrencppInjectTSAN.cmake"
          CACHE INTERNAL "")
endif()

# Enable warnings from includes
set(concurrencpp_INCLUDE_WITHOUT_SYSTEM ON CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(concurrencpp)

# ---- Test ----

enable_testing()

set(test_sources
        source/infra/assertions.cpp
        source/infra/tester.cpp
        source/utils/object_observer.cpp
)

set(test_headers
        include/infra/assertions.h
        include/infra/tester.h
        include/utils/object_observer.h
        include/utils/random.h
        include/utils/custom_exception.h
        include/utils/executor_shutdowner.h
        include/utils/test_generators.h
        include/utils/throwing_executor.h
        include/utils/test_ready_result.h)

# add_test(NAME <name> PATH <path> [PROPERTIES ...])
#
# Add test with the name <name> with the source file at
# <path> with project specific options that outputs the
# exectuable <name>.exe .
#
# Additional properties may be forwarded to `set_tests_properties` using the
# PROPERTIES arguments.
# See: https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-tests
#
function(add_test)
  cmake_parse_arguments(TEST "" "NAME;PATH" "PROPERTIES" ${ARGN})

  set(target "${TEST_NAME}")
  set(test_name "${TEST_NAME}")

  add_executable(${target} ${TEST_PATH} ${test_headers} ${test_sources})

  target_link_libraries(${target} PRIVATE concurrencpp::concurrencpp)

  target_compile_features(${target} PRIVATE cxx_std_20)

  target_coroutine_options(${target})

  target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/include")

  # Call the original add_test
  _add_test(NAME ${test_name} COMMAND ${target})

  set_property(TEST ${test_name} PROPERTY RUN_SERIAL YES)

  if(TEST_PROPERTIES)
    set_tests_properties(${test_name} PROPERTIES ${TEST_PROPERTIES})
  endif()
endfunction()

add_test(NAME task_tests PATH source/tests/task_tests.cpp)
add_test(NAME runtime_tests PATH source/tests/runtime_tests.cpp)

add_test(NAME inline_executor_tests PATH source/tests/executor_tests/inline_executor_tests.cpp)
add_test(NAME manual_executor_tests PATH source/tests/executor_tests/manual_executor_tests.cpp)
add_test(NAME thread_executor_tests PATH source/tests/executor_tests/thread_executor_tests.cpp)
add_test(NAME thread_pool_executor_tests PATH source/tests/executor_tests/thread_pool_executor_tests.cpp)
add_test(NAME worker_thread_executor_tests PATH source/tests/executor_tests/worker_thread_executor_tests.cpp)

add_test(NAME result_tests PATH source/tests/result_tests/result_tests.cpp)
add_test(NAME result_resolving_tests PATH source/tests/result_tests/result_resolve_tests.cpp)
add_test(NAME result_awaiting_tests PATH source/tests/result_tests/result_await_tests.cpp)

add_test(NAME shared_result_tests PATH source/tests/result_tests/shared_result_tests.cpp)
add_test(NAME shared_result_resolving_tests PATH source/tests/result_tests/shared_result_resolve_tests.cpp)
add_test(NAME shared_result_awaiting_tests PATH source/tests/result_tests/shared_result_await_tests.cpp)

add_test(NAME make_result_tests PATH source/tests/result_tests/make_result_tests.cpp)
add_test(NAME result_promise_tests PATH source/tests/result_tests/result_promise_tests.cpp)
add_test(NAME when_all_tests PATH source/tests/result_tests/when_all_tests.cpp)
add_test(NAME when_any_tests PATH source/tests/result_tests/when_any_tests.cpp)

add_test(NAME coroutine_promise_tests PATH source/tests/coroutine_tests/coroutine_promise_tests.cpp)
add_test(NAME coroutine_tests PATH source/tests/coroutine_tests/coroutine_tests.cpp)

add_test(NAME timer_queue_tests PATH source/tests/timer_tests/timer_queue_tests.cpp)
add_test(NAME timer_tests PATH source/tests/timer_tests/timer_tests.cpp)

if(NOT ENABLE_THREAD_SANITIZER)
  return()
endif()

add_test(NAME tsan_executor_tests PATH source/thread_sanitizer/executors.cpp)
add_test(NAME tsan_result_tests PATH source/thread_sanitizer/result.cpp)
add_test(NAME tsan_shared_result_tests PATH source/thread_sanitizer/shared_result.cpp)
add_test(NAME tsan_when_all_tests PATH source/thread_sanitizer/when_all.cpp)
add_test(NAME tsan_when_any_tests PATH source/thread_sanitizer/when_any.cpp)
add_test(NAME tsan_fibonacci PATH source/thread_sanitizer/fibonacci.cpp)
add_test(NAME tsan_quick_sort PATH source/thread_sanitizer/quick_sort.cpp)
add_test(NAME tsan_matrix_multiplication PATH source/thread_sanitizer/matrix_multiplication.cpp)