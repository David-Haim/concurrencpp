# Specify this file as CMAKE_TOOLCHAIN_FILE when invoking CMake with Clang
# to link to libc++ instead of libstdc++

string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -stdlib=libc++ -lc++abi")
