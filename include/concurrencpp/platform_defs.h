#ifndef CONCURRENCPP_PLATFORM_DEFS_H
#define CONCURRENCPP_PLATFORM_DEFS_H

#if defined(__MINGW32__)
#    define CRCPP_MINGW_OS
#elif defined(_WIN32)
#    define CRCPP_WIN_OS
#elif defined(unix) || defined(__unix__) || defined(__unix)
#    define CRCPP_UNIX_OS
#elif defined(__APPLE__) || defined(__MACH__)
#    define CRCPP_MAC_OS
#elif defined(__FreeBSD__)
#    define CRCPP_FREE_BSD_OS
#elif defined(__ANDROID__)
#    define CRCPP_ANDROID_OS
#endif

#if defined(__clang__)
#    define CRCPP_CLANG_COMPILER
#elif defined(__GNUC__) || defined(__GNUG__)
#    define CRCPP_GCC_COMPILER
#elif defined(_MSC_VER)
#    define CRCPP_MSVC_COMPILER
#endif

#if !defined(NDEBUG) || defined(_DEBUG)
#    define CRCPP_DEBUG_MODE
#endif

#if defined(CRCPP_WIN_OS)
#    if defined(CRCPP_EXPORT_API)
#        define CRCPP_API __declspec(dllexport)
#    elif defined(CRCPP_IMPORT_API)
#        define CRCPP_API __declspec(dllimport)
#    endif
#elif (defined(CRCPP_EXPORT_API) || defined(CRCPP_IMPORT_API)) && __has_cpp_attribute(gnu::visibility)
#    define CRCPP_API __attribute__((visibility("default")))
#endif

#if !defined(CRCPP_API)
#    define CRCPP_API
#endif

#include <exception>

#if defined(_LIBCPP_VERSION)
#    define CRCPP_LIBCPP_LIB
#endif

#endif  // PLATFORM_DEFS_H
