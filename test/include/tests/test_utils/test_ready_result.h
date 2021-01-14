#ifndef CONCURRENCPP_TEST_READY_RESULT_H
#define CONCURRENCPP_TEST_READY_RESULT_H

#include "concurrencpp/concurrencpp.h"

#include "result_factory.h"
#include "helpers/assertions.h"

#include <algorithm>

namespace concurrencpp::tests {
    template<class type>
    void test_same_ref_shared_result(::concurrencpp::shared_result<type>& result) noexcept {
        const auto value_ptr = std::addressof(result.get());

        for (size_t i = 0; i < 8; i++) {
            assert_equal(value_ptr, std::addressof(result.get()));
        }
    }

    template<class type>
    void test_same_exception_ptr_shared_result(::concurrencpp::shared_result<type>& result) noexcept {
        std::exception_ptr exception_ptr;

        try {
            result.get();
        } catch (...) {
            exception_ptr = std::current_exception();
        }

        assert_not_equal(exception_ptr, nullptr);

        for (size_t i = 0; i < 8; i++) {
            std::exception_ptr comparer;

            try {
                result.get();
            } catch (...) {
                comparer = std::current_exception();
            }

            assert_equal(exception_ptr, comparer);
        }
    }
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    struct costume_exception : public std::exception {
        const intptr_t id;

        costume_exception(intptr_t id) noexcept : id(id) {}
    };
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    template<class type>
    void test_ready_result(::concurrencpp::result<type> result, const type& o) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            assert_equal(result.get(), o);
        } catch (...) {
            assert_true(false);
        }
    }

    template<class type>
    void test_ready_result(::concurrencpp::shared_result<type> result, const type& o) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            assert_equal(result.get(), o);
        } catch (...) {
            assert_true(false);
        }

        test_same_ref_shared_result(result);
    }

    template<class type>
    void test_ready_result(::concurrencpp::result<type> result, std::reference_wrapper<typename std::remove_reference_t<type>> ref) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            assert_equal(&result.get(), &ref.get());
        } catch (...) {
            assert_true(false);
        }
    }

    template<class type>
    void test_ready_result(::concurrencpp::shared_result<type> result, std::reference_wrapper<typename std::remove_reference_t<type>> ref) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            assert_equal(&result.get(), &ref.get());
        } catch (...) {
            assert_true(false);
        }

        test_same_ref_shared_result(result);
    }

    template<class type>
    void test_ready_result(::concurrencpp::result<type> result) {
        test_ready_result<type>(std::move(result), result_factory<type>::get());
    }

    template<>
    inline void test_ready_result(::concurrencpp::result<void> result) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            result.get();  // just make sure no exception is thrown.
        } catch (...) {
            assert_true(false);
        }
    }

    template<class type>
    void test_ready_result(::concurrencpp::shared_result<type> result) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            assert_equal(result.get(), result_factory<type>::get());
        } catch (...) {
            assert_true(false);
        }

        test_same_ref_shared_result(result);
    }

    template<>
    inline void test_ready_result(::concurrencpp::shared_result<void> result) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::value);

        try {
            result.get();  // just make sure no exception is thrown.
        } catch (...) {
            assert_true(false);
        }
    }

    template<class type>
    void test_ready_result_costume_exception(concurrencpp::result<type> result, const intptr_t id) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::exception);

        try {
            result.get();
        } catch (costume_exception e) {
            return assert_equal(e.id, id);
        } catch (...) {
        }

        assert_true(false);
    }

    template<class type>
    void test_ready_result_costume_exception(concurrencpp::shared_result<type> result, const intptr_t id) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::exception);

        try {
            result.get();
        } catch (costume_exception e) {
            assert_equal(e.id, id);
            test_same_exception_ptr_shared_result(result);
            return;
        } catch (...) {
        }

        assert_true(false);
    }
}  // namespace concurrencpp::tests

#endif
