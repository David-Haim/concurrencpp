# Don't ignore empty list elements
cmake_policy(SET CMP0007 NEW)

set(args "")
foreach(n RANGE ${CMAKE_ARGC})
  if(NOT "${CMAKE_ARGV${n}}" STREQUAL "")
    list(APPEND args "${CMAKE_ARGV${n}}")
  endif()
endforeach()

list(FIND args "--" index)
if(index EQUAL -1)
  message(FATAL_ERROR "No -- divider found in arguments list")
else()
  set(temp "${args}")
  math(EXPR index "${index} + 1")
  list(SUBLIST temp ${index} -1 args)
endif()

list(POP_FRONT args source build os cmake ninja cores)

include(cmake/exec.cmake)
include(cmake/setCiVars.cmake)

exec(${cmake} -S ${source} -B ${build} -G Ninja -D CMAKE_MAKE_PROGRAM=${ninja}
-D CMAKE_BUILD_TYPE=RelWithDebInfo -D CMAKE_INSTALL_PREFIX=build/prefix ${flags} ${args})

exec(${cmake} --build ${build} --config RelWithDebInfo -j ${cores})
