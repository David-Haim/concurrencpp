#ifndef CONCURRENCPP_TEST_READY_RESULT_H
#define CONCURRENCPP_TEST_READY_RESULT_H

#include "concurrencpp/concurrencpp.h"

#include "infra/assertions.h"
#include "utils/test_generators.h"
#include "utils/custom_exception.h"

#include <algorithm>

namespace concurrencpp::tests {
    template<class type>
    void test_same_ref_shared_result(::concurrencpp::shared_result<type>& result) noexcept {
        const auto value_ptr = std::addressof(result.get());

        for (size_t i = 0; i < 8; i++) {
            assert_equal(value_ptr, std::addressof(result.get()));
        }
    }
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
    void test_ready_result(::concurrencpp::shared_result<type> result,
                           std::reference_wrapper<typename std::remove_reference_t<type>> ref) {
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
        test_ready_result<type>(std::move(result), value_gen<type>::default_value());
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
            assert_equal(result.get(), value_gen<type>::default_value());
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
    void test_ready_result_custom_exception(concurrencpp::result<type> result, const intptr_t id) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::exception);

        try {
            result.get();
        } catch (const custom_exception& e) {
            return assert_equal(e.id, id);
        } catch (...) {
        }

        assert_true(false);
    }

    template<class type>
    void test_ready_result_custom_exception(concurrencpp::shared_result<type> result, const intptr_t id) {
        assert_true(static_cast<bool>(result));
        assert_equal(result.status(), concurrencpp::result_status::exception);

        for (size_t i = 0; i < 10; i++) {
            try {
                result.get();
            } catch (const custom_exception& e) {
                assert_equal(e.id, id);
                if (i == 9) {
                    return;
                }

            } catch (...) {
            }
        }

        assert_true(false);
    }
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    template<class type, class consumer_type>
    void test_result_array(std::vector<result<type>> results, consumer_type&& consumer, value_gen<type> converter) {
        for (size_t i = 0; i < results.size(); i++) {
            if constexpr (!std::is_same_v<void, type>) {
                test_ready_result(consumer(std::move(results[i])), converter.value_of(i));
            } else {
                test_ready_result(consumer(std::move(results[i])));
            }
        }
    }

    template<class type, class consumer_type>
    void test_exceptional_array(std::vector<result<type>> results, consumer_type&& consumer) {
        for (size_t i = 0; i < results.size(); i++) {
            test_ready_result_custom_exception(consumer(std::move(results[i])), i);
        }
    }

    template<class type, class consumer_type>
    void test_shared_result_array(std::vector<shared_result<type>> results, consumer_type&& consumer, value_gen<type> converter) {
        for (size_t i = 0; i < results.size(); i++) {
            if constexpr (!std::is_same_v<void, type>) {
                test_ready_result(consumer(std::move(results[i])), converter.value_of(i));
            } else {
                test_ready_result(consumer(std::move(results[i])));
            }
        }
    }

    template<class type, class consumer_type>
    void test_shared_result_exceptional_array(std::vector<shared_result<type>> results, consumer_type&& consumer) {
        for (size_t i = 0; i < results.size(); i++) {
            test_ready_result_custom_exception(consumer(std::move(results[i])), i);
        }
    }
}  // namespace concurrencpp::tests

#endif
