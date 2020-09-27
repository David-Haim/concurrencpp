cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(concurrencpp
        VERSION 0.0.6
        LANGUAGES CXX)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(coroutineOptions)

# ---- Options ----

option(CONCURRENCPP_ENABLE_THREAD_SANITIZER "\
Build concurrencpp with LLVM thread sanitizer. \
Does not have an effect if the compiler is not Clang based." OFF)

# ---- Warning guard ----

# Protect dependents from this project's warnings if the guard isn't disabled
set(concurrencpp_warning_guard "SYSTEM")
if(concurrencpp_INCLUDE_WITHOUT_SYSTEM)
  set(concurrencpp_warning_guard "")
endif()

# ---- Declare library ----

set(concurrencpp_sources
        source/executors/executor.cpp
        source/executors/manual_executor.cpp
        source/executors/thread_executor.cpp
        source/executors/thread_pool_executor.cpp
        source/executors/worker_thread_executor.cpp
        source/results/promises.cpp
        source/results/result_core.cpp
        source/runtime/runtime.cpp
        source/threads/thread.cpp
        source/timers/timer.cpp
        source/timers/timer_queue.cpp)

set(concurrencpp_headers
        include/concurrencpp/concurrencpp.h
        include/concurrencpp/errors.h
        include/concurrencpp/forward_declerations.h
        include/concurrencpp/platform_defs.h
        include/concurrencpp/executors/constants.h
        include/concurrencpp/executors/derivable_executor.h
        include/concurrencpp/executors/executor.h
        include/concurrencpp/executors/executor_all.h
        include/concurrencpp/executors/inline_executor.h
        include/concurrencpp/executors/manual_executor.h
        include/concurrencpp/executors/thread_executor.h
        include/concurrencpp/executors/thread_pool_executor.h
        include/concurrencpp/executors/worker_thread_executor.h
        include/concurrencpp/results/constants.h
        include/concurrencpp/results/executor_exception.h
        include/concurrencpp/results/make_result.h
        include/concurrencpp/results/promises.h
        include/concurrencpp/results/result.h
        include/concurrencpp/results/result_awaitable.h
        include/concurrencpp/results/result_core.h
        include/concurrencpp/results/result_fwd_declerations.h
        include/concurrencpp/results/when_result.h
        include/concurrencpp/runtime/constants.h
        include/concurrencpp/runtime/runtime.h
        include/concurrencpp/threads/thread.h
        include/concurrencpp/timers/constants.h
        include/concurrencpp/timers/timer.h
        include/concurrencpp/timers/timer_queue.h
        include/concurrencpp/utils/bind.h)

add_library(concurrencpp STATIC ${concurrencpp_headers} ${concurrencpp_sources})
add_library(concurrencpp::concurrencpp ALIAS concurrencpp)

target_include_directories(concurrencpp
        ${concurrencpp_warning_guard}
        PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")

target_compile_features(concurrencpp PUBLIC cxx_std_20)

target_coroutine_options(concurrencpp)

find_library(LIBRT NAMES rt DOC "Path to the Real Time shared library")
target_link_libraries(concurrencpp PUBLIC "$<$<BOOL:${LIBRT}>:${LIBRT}>")

if(CONCURRENCPP_ENABLE_THREAD_SANITIZER AND CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(concurrencpp PRIVATE -fsanitize=thread)
endif()

# ---- Install ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(concurrencpp_directory "concurrencpp-${PROJECT_VERSION}")
set(concurrencpp_include_directory "${CMAKE_INSTALL_INCLUDEDIR}/${concurrencpp_directory}")

install(TARGETS concurrencpp
        EXPORT concurrencppTargets
        ARCHIVE #
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT concurrencpp_Development
        INCLUDES #
        DESTINATION "${concurrencpp_include_directory}")

set(concurrencpp_install_cmakedir
        "${CMAKE_INSTALL_LIBDIR}/cmake/${concurrencpp_directory}")

write_basic_package_version_file(
        concurrencppConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMinorVersion
        ARCH_INDEPENDENT)

install(EXPORT concurrencppTargets
        NAMESPACE concurrencpp::
        DESTINATION "${concurrencpp_install_cmakedir}")

install(FILES
        "${PROJECT_SOURCE_DIR}/cmake/concurrencppConfig.cmake"
        "${PROJECT_BINARY_DIR}/concurrencppConfigVersion.cmake"
        DESTINATION "${concurrencpp_install_cmakedir}")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION "${concurrencpp_include_directory}")
