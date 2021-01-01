# Sets the required compiler options for the given target, or stops CMake if the
# current compiler doesn't support coroutines.
#
function(target_coroutine_options TARGET)
  if(MSVC)
    target_compile_options(${TARGET} PUBLIC /std:c++latest /permissive-)
    return()
  endif()

  find_package(Threads REQUIRED)
  target_link_libraries(${TARGET} PRIVATE Threads::Threads)

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${TARGET} PUBLIC -stdlib=libc++ -fcoroutines-ts)
    target_link_options(${TARGET} PUBLIC -stdlib=libc++)
    set_target_properties(${TARGET} PROPERTIES CXX_EXTENSIONS NO)
  else()
    message(FATAL_ERROR "Compiler not supported: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endfunction()
