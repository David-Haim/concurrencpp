#ifndef CONCURRENCPP_RESULT_FWD_DECLERATIONS_H
#define CONCURRENCPP_RESULT_FWD_DECLERATIONS_H

#include "concurrencpp/forward_declerations.h"

#include <memory>
#include <utility>
#include <experimental/coroutine>

namespace concurrencpp {
    template<class type>
    class result;
    template<class type>
    class result_promise;

    template<class type>
    class awaitable;
    template<class type>
    class via_awaitable;
    template<class type>
    class resolve_awaitable;
    template<class type>
    class resolve_via_awaitable;

    struct executor_tag {};

    struct null_result {};

    enum class result_status { idle, value, exception };
}  // namespace concurrencpp

namespace concurrencpp::details {
    template<class type>
    class result_state;

    struct executor_bulk_tag {};

    class when_result_helper;

    enum class when_any_status { set, result_ready };
}  // namespace concurrencpp::details

#endif
