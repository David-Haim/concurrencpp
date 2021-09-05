#ifndef CONCURRENCPP_COROUTINE_H
#define CONCURRENCPP_COROUTINE_H

#include "../platform_defs.h"

#if defined(CRCPP_MSVC_COMPILER) || defined(CRCPP_GCC_COMPILER)
#    include <coroutine>
#    define CRCPP_COROUTINE_NAMESPACE std
#elif defined(CRCPP_CLANG_COMPILER)
#    include <experimental/coroutine>
#    define CRCPP_COROUTINE_NAMESPACE std::experimental
#endif

namespace concurrencpp::details {
    template<class promise_type>
    using coroutine_handle = CRCPP_COROUTINE_NAMESPACE::coroutine_handle<promise_type>;
    using suspend_never = CRCPP_COROUTINE_NAMESPACE::suspend_never;
    using suspend_always = CRCPP_COROUTINE_NAMESPACE::suspend_always;
}  // namespace concurrencpp::details

#endif