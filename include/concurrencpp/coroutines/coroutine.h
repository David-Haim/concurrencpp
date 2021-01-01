#ifndef CONCURRENCPP_COROUTINE_H
#define CONCURRENCPP_COROUTINE_H

#include "../platform_defs.h"

#ifdef CRCPP_MSVC_COMPILER

#    include <coroutine>

namespace concurrencpp::details {
    template<class promise_type>
    using coroutine_handle = std::coroutine_handle<promise_type>;
    using suspend_never = std::suspend_never;
    using suspend_always = std::suspend_always;
}  // namespace concurrencpp::details

#elif defined(CRCPP_CLANG_COMPILER)

#    include <experimental/coroutine>

namespace concurrencpp::details {
    template<class promise_type>
    using coroutine_handle = std::experimental::coroutine_handle<promise_type>;
    using suspend_never = std::experimental::suspend_never;
    using suspend_always = std::experimental::suspend_always;
}  // namespace concurrencpp::details

#endif

#endif