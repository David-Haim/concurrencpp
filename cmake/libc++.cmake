# Specify this file as CMAKE_TOOLCHAIN_FILE when invoking CMake with Clang
# to link to libc++ instead of libstdc++

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
