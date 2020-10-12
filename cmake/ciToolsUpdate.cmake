set(cmake_version $ENV{CMAKE_VERSION})
set(ninja_version $ENV{NINJA_VERSION})

if(RUNNER_OS STREQUAL "Windows")
  set(ninja_suffix "win.zip")
  set(cmake_suffix "win64-x64.zip")
  set(cmake_dir "cmake-${cmake_version}-win64-x64/bin")
elseif(RUNNER_OS STREQUAL "Linux")
  set(ninja_suffix "linux.zip")
  set(cmake_suffix "Linux-x86_64.tar.gz")
  set(cmake_dir "cmake-${cmake_version}-Linux-x86_64/bin")
elseif(RUNNER_OS STREQUAL "macOS")
  set(ninja_suffix "mac.zip")
  set(cmake_suffix "Darwin-x86_64.tar.gz")
  set(cmake_dir "cmake-${cmake_version}-Darwin-x86_64/CMake.app/Contents/bin")
endif()

set(cmake_url "https://github.com/Kitware/CMake/releases/download/v${cmake_version}/cmake-${cmake_version}-${cmake_suffix}")
file(DOWNLOAD "${cmake_url}" ./cmake.zip)
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ./cmake.zip OUTPUT_QUIET)
message(STATUS "Installed CMake")

set(ninja_url "https://github.com/ninja-build/ninja/releases/download/v${ninja_version}/ninja-${ninja_suffix}")
file(DOWNLOAD "${ninja_url}" ./ninja.zip)
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ./ninja.zip OUTPUT_QUIET)
message(STATUS "Installed Ninja")

set(export_script "#!/bin/sh\n")

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/${cmake_dir}" cmake_dir)
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
