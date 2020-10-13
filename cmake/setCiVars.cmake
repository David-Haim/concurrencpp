if (os MATCHES "^windows")
  execute_process(
    COMMAND "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Auxiliary/Build/vcvars64.bat" && set
    OUTPUT_FILE environment_script_output.txt
  )
  file(STRINGS environment_script_output.txt output_lines)
  foreach(line IN LISTS output_lines)
    if (line MATCHES "^([a-zA-Z0-9_-]+)=(.*)$")
      set(ENV{${CMAKE_MATCH_1}} "${CMAKE_MATCH_2}")
    endif()
  endforeach()
endif()
