# Inject sanitizer flag as a build requirement
#
macro(add_library TARGET)
  _add_library(${ARGV})

  if("${TARGET}" STREQUAL "concurrencpp")
    target_compile_options(concurrencpp PUBLIC -fsanitize=thread)
    target_link_options(concurrencpp PUBLIC -fsanitize=thread)
  endif()
endmacro()
