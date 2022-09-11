#ifndef CONCURRENCPP_COROUTINE_H
#define CONCURRENCPP_COROUTINE_H

#include "../platform_defs.h"

#if !__has_include(<coroutine>) && __has_include(<experimental/coroutine>)

#    include <experimental/coroutine>
#    define CRCPP_COROUTINE_NAMESPACE std::experimental

#else

#    include <coroutine>
#    define CRCPP_COROUTINE_NAMESPACE std

#endif

namespace concurrencpp::details {
    template<class promise_type>
    using coroutine_handle = CRCPP_COROUTINE_NAMESPACE::coroutine_handle<promise_type>;
    using suspend_never = CRCPP_COROUTINE_NAMESPACE::suspend_never;
    using suspend_always = CRCPP_COROUTINE_NAMESPACE::suspend_always;
}  // namespace concurrencpp::details

#endif