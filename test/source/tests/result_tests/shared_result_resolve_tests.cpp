#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/test_ready_result.h"
#include "utils/throwing_executor.h"
#include "utils/executor_shutdowner.h"

namespace concurrencpp::tests {
    template<class type>
    void test_shared_result_resolve_impl();
    void test_shared_result_resolve();
}  // namespace concurrencpp::tests

using concurrencpp::result;
using concurrencpp::details::thread;

/*
 *  In this test suit, we need to check all the possible scenarios that result::resolve can have.
 *  Our tests are split into 2 branches: when the result is already ready at the moment of resolving and
 *  when it's not.
 *
 *  If the result is already ready, the test matrix looks like this:
 *  status[value, exception]
 *  Overall 2 scenarios
 *
 * If the result is not ready, the test matrix looks like this:
 * status[value, exception]
 * Overall 2 scenarios
 */

namespace concurrencpp::tests {
    template<class type, result_status status>
    struct test_await_ready_result {
        result<void> operator()();
    };

    template<class type>
    struct test_await_ready_result<type, result_status::value> {
        result<void> operator()() {
            auto result = result_gen<type>::ready();
            concurrencpp::shared_result<type> sr(std::move(result));

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await sr.resolve();

            const auto thread_id_1 = thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_equal(thread_id_0, thread_id_1);
            test_ready_result(std::move(done_result));
        }
    };

    template<class type>
    struct test_await_ready_result<type, result_status::exception> {
        result<void> operator()() {
            const auto id = 1234567;
            auto result = make_exceptional_result<type>(custom_exception(id));
            concurrencpp::shared_result<type> sr(std::move(result));

            const auto thread_id_0 = concurrencpp::details::thread::get_current_virtual_id();

            auto done_result = co_await sr.resolve();

            const auto thread_id_1 = concurrencpp::details::thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_equal(thread_id_0, thread_id_1);
            test_ready_result_custom_exception(std::move(done_result), id);
        }
    };

    template<class type, result_status status>
    struct test_await_not_ready_result {
        result<void> operator()(std::shared_ptr<thread_executor> executor);
    };

    template<class type>
    struct test_await_not_ready_result<type, result_status::value> {

       private:
        uintptr_t m_setting_thread_id = 0;
        uintptr_t m_resuming_thread_id = 0;

        result<void> inner_task(std::shared_ptr<manual_executor> manual_executor) {
            auto result = manual_executor->submit([]() -> decltype(auto) {
                return value_gen<type>::default_value();
            });
            concurrencpp::shared_result<type> sr(std::move(result));

            auto done_result = co_await sr.resolve();

            m_resuming_thread_id = thread::get_current_virtual_id();

            test_ready_result(std::move(done_result));
        }

       public:
        result<void> operator()(std::shared_ptr<manual_executor> manual_executor, std::shared_ptr<thread_executor> thread_executor) {
            assert_true(manual_executor->empty());

            auto result = inner_task(manual_executor);

            co_await thread_executor->submit([this, manual_executor] {
                m_setting_thread_id = thread::get_current_virtual_id();
                assert_true(manual_executor->loop_once());
            });

            co_await result;

            assert_equal(m_setting_thread_id, m_resuming_thread_id);
        }
    };

    template<class type>
    struct test_await_not_ready_result<type, result_status::exception> {

       private:
        uintptr_t m_setting_thread_id = 0;
        uintptr_t m_resuming_thread_id = 0;

        result<void> inner_task(std::shared_ptr<manual_executor> manual_executor) {
            const auto id = 1234567;
            auto result = manual_executor->submit([id]() -> decltype(auto) {
                throw custom_exception(id);
                return value_gen<type>::default_value();
            });
            concurrencpp::shared_result<type> sr(std::move(result));

            auto done_result = co_await sr.resolve();

            m_resuming_thread_id = thread::get_current_virtual_id();

            test_ready_result_custom_exception(std::move(done_result), id);
        }

       public:
        result<void> operator()(std::shared_ptr<manual_executor> manual_executor, std::shared_ptr<thread_executor> thread_executor) {
            assert_true(manual_executor->empty());

            auto result = inner_task(manual_executor);

            co_await thread_executor->submit([this, manual_executor] {
                m_setting_thread_id = concurrencpp::details::thread::get_current_virtual_id();
                assert_true(manual_executor->loop_once());
            });

            co_await result;

            assert_equal(m_setting_thread_id, m_resuming_thread_id);
        }
    };

}  // namespace concurrencpp::tests

template<class type>
void concurrencpp::tests::test_shared_result_resolve_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                shared_result<type>().resolve();
            },
            concurrencpp::details::consts::k_shared_result_resolve_error_msg);
    }

    // resolve can be called multiple times
    {
        shared_result<type> sr(result_gen<type>::ready());

        for (size_t i = 0; i < 6; i++) {
            sr.resolve();
            assert_true(sr);
        }
    }

    auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
    auto manual_executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner es0(thread_executor), es1(manual_executor);

    test_await_ready_result<type, result_status::value>()().get();
    test_await_ready_result<type, result_status::exception>()().get();
    test_await_not_ready_result<type, result_status::value>()(manual_executor, thread_executor).get();
    test_await_not_ready_result<type, result_status::exception>()(manual_executor, thread_executor).get();
}

void concurrencpp::tests::test_shared_result_resolve() {
    test_shared_result_resolve_impl<int>();
    test_shared_result_resolve_impl<std::string>();
    test_shared_result_resolve_impl<void>();
    test_shared_result_resolve_impl<int&>();
    test_shared_result_resolve_impl<std::string&>();
}
using namespace concurrencpp::tests;

int main() {
    tester tester("shared_result::resolve");

    tester.add_step("reslove", test_shared_result_resolve);

    tester.launch_test();
    return 0;
}