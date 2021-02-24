#ifndef CONCURRENCPP_PLATFORM_DEFS_H
#define CONCURRENCPP_PLATFORM_DEFS_H

#if defined(_WIN32)
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

#endif  // PLATFORM_DEFS_H
