# Sets the required compiler options for the given target, or stops CMake if the
# current compiler doesn't support coroutines.
#
function(target_coroutine_options TARGET)
  if(MSVC)
    target_compile_options(${TARGET} PUBLIC /std:c++latest /permissive-)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_target_properties(${TARGET} PROPERTIES CXX_EXTENSIONS NO)
  else()
    message(FATAL_ERROR "Compiler not supported: ${CMAKE_CXX_COMPILER_ID}")
  endif()
endfunction()
