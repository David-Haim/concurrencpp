#ifndef CONCURRENCPP_CACHE_LINE_H
#define CONCURRENCPP_CACHE_LINE_H

#include "concurrencpp/platform_defs.h"

#include <new>

#if !defined(CRCPP_MAC_OS) && defined(__cpp_lib_hardware_interference_size)
#    if defined(CRCPP_GCC_COMPILER) 
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Winterference-size"
#    endif
constexpr inline std::size_t CRCPP_CACHE_LINE_ALIGNMENT = std::hardware_destructive_interference_size;
#    if defined(CRCPP_GCC_COMPILER) 
#        pragma GCC diagnostic pop
#    endif
#else
constexpr inline std::size_t CRCPP_CACHE_LINE_ALIGNMENT = 64;
#endif

#endif
