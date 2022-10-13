set(cmake_version $ENV{CMAKE_VERSION})
set(ninja_version $ENV{NINJA_VERSION})

if(RUNNER_OS STREQUAL "Windows")
  set(ninja_suffix "win.zip")
  set(cmake_ext "zip")
  set(cmake_os "win64")
  set(cmake_arch "x64")
  set(cmake_bindir "bin")
else()
  set(cmake_ext "tar.gz")
  set(cmake_arch "x86_64")
  if(RUNNER_OS STREQUAL "Linux")
    set(ninja_suffix "linux.zip")
    set(cmake_bindir "bin")
    if("${cmake_version}" VERSION_GREATER_EQUAL "3.20.0")
      set(cmake_os "linux")
    else()
      set(cmake_os "Linux")
    endif()
  elseif(RUNNER_OS STREQUAL "macOS")
    set(ninja_suffix "mac.zip")
    set(cmake_bindir "CMake.app/Contents/bin")
    if("${cmake_version}" VERSION_GREATER_EQUAL "3.20.0")
      set(cmake_os "macos")
      set(cmake_arch "universal")
    else()
      set(cmake_os "Darwin")
    endif()
  endif()
endif()

set(cmake_base "cmake-${cmake_version}-${cmake_os}-${cmake_arch}")

set(cmake_url
    "https://github.com/Kitware/CMake/releases/download/v${cmake_version}/${cmake_base}.${cmake_ext}"
)
file(DOWNLOAD "${cmake_url}" ./cmake.zip)
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ./cmake.zip OUTPUT_QUIET)
message(STATUS "Installed CMake")

set(ninja_url "https://github.com/ninja-build/ninja/releases/download/v${ninja_version}/ninja-${ninja_suffix}")
file(DOWNLOAD "${ninja_url}" ./ninja.zip)
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ./ninja.zip OUTPUT_QUIET)
message(STATUS "Installed Ninja")

set(export_script "#!/bin/sh\n")

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/${cmake_base}/${cmake_bindir}"
     cmake_dir)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/ninja" ninja_out)

function(echo MESSAGE)
  execute_process(COMMAND ${CMAKE_COMMAND} -E echo "${MESSAGE}")
endfunction()

set(export_script "${export_script}export CTEST=\"${cmake_dir}/ctest\"\n")
echo("::set-output name=ctest::${cmake_dir}/ctest")
message(STATUS "ctest path: ${cmake_dir}/ctest")

set(export_script "${export_script}export CMAKE=\"${cmake_dir}/cmake\"\n")
echo("::set-output name=cmake::${cmake_dir}/cmake")
message(STATUS "cmake path: ${cmake_dir}/cmake")

set(export_script "${export_script}export NINJA=\"${ninja_out}\"\n")
echo("::set-output name=ninja::${ninja_out}")
message(STATUS "ninja path: ${ninja_out}")

file(WRITE export.sh "${export_script}")

if (NOT RUNNER_OS STREQUAL "Windows")
  execute_process(COMMAND chmod +x ninja export.sh "${cmake_dir}/cmake" "${cmake_dir}/ctest")
endif()
