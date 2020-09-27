cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(concurrencppTests LANGUAGES CXX)

# ---- Add root project ----

# Enable warnings from includes
set(concurrencpp_INCLUDE_WITHOUT_SYSTEM ON CACHE INTERNAL "")

include(FetchContent)
FetchContent_Declare(concurrencpp SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
FetchContent_MakeAvailable(concurrencpp)

# ---- Test ----

enable_testing()

# add_test(NAMESPACE <namespace> NAME <name>)
#
# Add test with the name <namespace>.<name> with the source file at
# source/<namespace>/<name>.cpp with project specific options that outputs the
# exectuable <namespace>_<name>.
#
function(add_test)
  cmake_parse_arguments(TEST "" "NAMESPACE;NAME" "" ${ARGN})

  set(target "${TEST_NAMESPACE}_${TEST_NAME}")

  add_executable(${target} "source/${TEST_NAMESPACE}/${TEST_NAME}.cpp")

  target_link_libraries(${target} PRIVATE concurrencpp::concurrencpp)

  target_compile_features(${target} PRIVATE cxx_std_20)

  # Call the original add_test
  _add_test(NAME "${TEST_NAMESPACE}.${TEST_NAME}" COMMAND ${target})
endfunction()

# TODO: add tests
