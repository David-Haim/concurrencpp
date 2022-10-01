#ifndef CONCURRENCPP_TEST_READY_LAZY_RESULT_H
#define CONCURRENCPP_TEST_READY_LAZY_RESULT_H

#include "concurrencpp/concurrencpp.h"

#include "infra/assertions.h"
#include "utils/test_generators.h"
#include "utils/custom_exception.h"

#include <algorithm>

namespace concurrencpp::tests {
    template<class type>
    null_result test_ready_lazy_result(::concurrencpp::lazy_result<type> result, const type& o) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            assert_equal(co_await result, o);
        } catch (...) {
            assert_true(false);
        }
    }

    template<class type>
    null_result test_ready_lazy_result(::concurrencpp::lazy_result<type> result,
                                       std::reference_wrapper<typename std::remove_reference_t<type>> ref) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            auto& result_ref = co_await result;
            assert_equal(&result_ref, &ref.get());
        } catch (...) {
            assert_true(false);
        }
    }

    template<class type>
    null_result test_ready_lazy_result(::concurrencpp::lazy_result<type> result) {
        return test_ready_lazy_result<type>(std::move(result), value_gen<type>::default_value());
    }

    template<>
    inline null_result test_ready_lazy_result(::concurrencpp::lazy_result<void> result) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            co_await result;  // just make sure no exception is thrown.
        } catch (...) {
            assert_true(false);
        }
    }

    template<class type>
    null_result test_ready_lazy_result_custom_exception(concurrencpp::lazy_result<type> result, const intptr_t id) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::exception);

        try {
            co_await result;
        } catch (const custom_exception& e) {
            assert_equal(e.id, id);
            co_return;
        } catch (...) {
        }

        assert_true(false);
    }
}  // namespace concurrencpp::tests

#endif
