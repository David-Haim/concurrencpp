#ifndef CONCURRENCPP_RESULT_FWD_DECLARATIONS_H
#define CONCURRENCPP_RESULT_FWD_DECLARATIONS_H

#include "concurrencpp/forward_declarations.h"
#include "concurrencpp/coroutines/coroutine.h"

#include <memory>
#include <utility>

namespace concurrencpp {
    template<class type>
    class result;

    template<class type>
    class shared_result;

    template<class type>
    class lazy_result;

    template<class type>
    class result_promise;

    template<class type>
    class awaitable;

    template<class type>
    class resolve_awaitable;

    struct executor_tag {};

    struct null_result {};

    enum class result_status { idle, value, exception };
}  // namespace concurrencpp

namespace concurrencpp::details {
    class result_state_base;

    template<class type>
    class result_state;

    class shared_result_state_base;

    template<class type>
    class shared_result_state;

    template<class type>
    class lazy_result_state;

    class when_result_helper;
    struct shared_result_helper;
}  // namespace concurrencpp::details

#endif
