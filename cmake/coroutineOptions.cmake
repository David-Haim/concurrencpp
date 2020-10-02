# Sets the required compiler options for the given target, or stops CMake if the
# current compiler doesn't support coroutines.
#
function(target_coroutine_options TARGET)
  if(MSVC)
    target_compile_options(${TARGET} PUBLIC /await)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    find_package(Threads REQUIRED)

    target_link_libraries(${TARGET} PRIVATE Threads::Threads)
    target_compile_options(${TARGET} PUBLIC -fcoroutines-ts)
  else()
    # -fcoroutines-ts works only in Clang and GCC doesn't yet have a flag
    # TODO: Add GCC flag(s) once it supports coroutines
    message(FATAL_ERROR "Compiler not supported: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endfunction()
