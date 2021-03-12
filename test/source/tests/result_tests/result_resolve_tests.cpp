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
    void test_result_resolve_impl();
    void test_result_resolve();

    template<class type>
    void test_result_resolve_via_impl();
    void test_result_resolve_via();

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

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await result.resolve();

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

            const auto thread_id_0 = concurrencpp::details::thread::get_current_virtual_id();

            auto done_result = co_await result.resolve();

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

            auto done_result = co_await result.resolve();

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

            auto done_result = co_await result.resolve();

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
void concurrencpp::tests::test_result_resolve_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                result<type>().resolve();
            },
            concurrencpp::details::consts::k_result_resolve_error_msg);
    }

    auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
    auto manual_executor = std::make_shared<concurrencpp::manual_executor>();
    executor_shutdowner es0(thread_executor), es1(manual_executor);

    test_await_ready_result<type, result_status::value>()().get();
    test_await_ready_result<type, result_status::exception>()().get();
    test_await_not_ready_result<type, result_status::value>()(manual_executor, thread_executor).get();
    test_await_not_ready_result<type, result_status::exception>()(manual_executor, thread_executor).get();
}

void concurrencpp::tests::test_result_resolve() {
    test_result_resolve_impl<int>();
    test_result_resolve_impl<std::string>();
    test_result_resolve_impl<void>();
    test_result_resolve_impl<int&>();
    test_result_resolve_impl<std::string&>();
}

/*
 *  In this test suit, we need to check all the possible scenarios that result::resolve_via can have.
 *  Our tests are split into 2 branches: when the result is already ready at the moment of resolving and
 *  when it's not.
 *
 *  If the result is already ready, the test matrix looks like this:
 *  status[value, exception] x force_rescheduling[true, false] x executor throws[true, false]
 *  Overall 8 scenarios
 *
 * If the result is not ready, the test matrix looks like this:
 * status[value, exception] x executor throws [true, false]
 * Overall 4 scenarios
 */

namespace concurrencpp::tests {
    template<class type, result_status status, bool force_rescheduling, bool executor_throws>
    struct test_resolve_via_ready_result {
        result<void> operator()(std::shared_ptr<thread_executor> executor);
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::value, false, false> {
        result<void> operator()(std::shared_ptr<thread_executor> executor) {
            // result is ready + force_rescheduling = false + executor doesn't throw
            // = inline execution, result is returned

            auto result = result_gen<type>::ready();

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await result.resolve_via(executor, false);

            const auto thread_id_1 = thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_equal(thread_id_0, thread_id_1);
            test_ready_result(std::move(done_result));
        }
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::value, true, false> {
        result<void> operator()(std::shared_ptr<thread_executor> executor) {
            // result is ready + force_rescheduling = true + executor doesn't throw
            // = rescheduling, result is returned
            auto result = result_gen<type>::ready();

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await result.resolve_via(executor, true);

            const auto thread_id_1 = thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_not_equal(thread_id_0, thread_id_1);
            test_ready_result(std::move(done_result));
        }
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::value, false, true> {
        result<void> operator()(std::shared_ptr<throwing_executor> executor) {
            // result is ready + force_rescheduling = false + executor throws
            // = inline execution, result is returned (executor doesn't have the chance to throw)
            auto result = result_gen<type>::ready();

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await result.resolve_via(executor, false);

            const auto thread_id_1 = thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_equal(thread_id_0, thread_id_1);
            test_ready_result(std::move(done_result));
        }
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::value, true, true> {
        result<void> operator()(std::shared_ptr<throwing_executor> executor) {
            // result is ready + force_rescheduling = true + executor throws
            // = inline execution, broken_task is thrown
            auto result = result_gen<type>::ready();

            const auto thread_id_0 = thread::get_current_virtual_id();

            try {
                auto aw = result.resolve_via(executor, true);
                co_await aw;
            } catch (const errors::broken_task&) {
                const auto thread_id_1 = thread::get_current_virtual_id();

                assert_false(static_cast<bool>(result));
                assert_equal(thread_id_0, thread_id_1);
                co_return;
            } catch (...) {
            }

            assert_false(true);
        }
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::exception, false, false> {
        result<void> operator()(std::shared_ptr<thread_executor> executor) {
            // result is ready (exception) + force_rescheduling = false + executor doesn't throw
            // = inline execution, asynchronous exception is returned
            const auto id = 1234567;
            auto result = make_exceptional_result<type>(custom_exception(id));

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await result.resolve_via(executor, false);

            const auto thread_id_1 = thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_equal(thread_id_0, thread_id_1);
            test_ready_result_custom_exception(std::move(done_result), id);
        }
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::exception, true, false> {
        result<void> operator()(std::shared_ptr<thread_executor> executor) {
            // result is ready (exception) + force_rescheduling = true + executor doesn't throw
            // = rescheduling, asynchronous exception is returned
            auto id = 1234567;
            auto result = make_exceptional_result<type>(custom_exception(id));

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await result.resolve_via(executor, true);

            const auto thread_id_1 = thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_not_equal(thread_id_0, thread_id_1);
            test_ready_result_custom_exception(std::move(done_result), id);
        }
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::exception, false, true> {
        result<void> operator()(std::shared_ptr<throwing_executor> executor) {
            // result is ready (exception) + force_rescheduling = false + executor throws
            // = inline execution, asynchronous exception is returned (executor doesn't have the chance to throw itself)
            const auto id = 1234567;
            auto result = make_exceptional_result<type>(custom_exception(id));

            const auto thread_id_0 = thread::get_current_virtual_id();

            auto done_result = co_await result.resolve_via(executor, false);

            const auto thread_id_1 = thread::get_current_virtual_id();

            assert_false(static_cast<bool>(result));
            assert_equal(thread_id_0, thread_id_1);
            test_ready_result_custom_exception(std::move(done_result), id);
        }
    };

    template<class type>
    struct test_resolve_via_ready_result<type, result_status::exception, true, true> {
        result<void> operator()(std::shared_ptr<throwing_executor> executor) {
            // result is ready (exception) + force_rescheduling = true + executor throws
            // = inline execution, broken_task exception is thrown
            auto result = result_gen<type>::exceptional();

            const auto thread_id_0 = thread::get_current_virtual_id();

            try {
                auto done_result = co_await result.resolve_via(executor, true);
            } catch (const errors::broken_task&) {
                const auto thread_id_1 = thread::get_current_virtual_id();

                assert_equal(thread_id_0, thread_id_1);
                assert_false(static_cast<bool>(result));
                co_return;
            } catch (...) {
            }

            assert_false(true);
        }
    };

    template<class type, result_status status, bool executor_throws>
    struct test_resolve_via_not_ready_result {
        result<void> operator()(std::shared_ptr<manual_executor> manual_executor, std::shared_ptr<thread_executor> thread_executor);
    };

    template<class type>
    struct test_resolve_via_not_ready_result<type, result_status::value, false> {
        // result is not ready (completes with a value) + executor doesn't throw
        // = rescheduling, result is returned

       private:
        uintptr_t m_launcher_thread_id = 0;
        uintptr_t m_setting_thread_id = 0;
        uintptr_t m_resuming_thread_id = 0;

        result<void> inner_task(std::shared_ptr<manual_executor> manual_executor, std::shared_ptr<thread_executor> thread_executor) {
            m_launcher_thread_id = thread::get_current_virtual_id();

            auto result = manual_executor->submit([]() -> decltype(auto) {
                return value_gen<type>::default_value();
            });

            auto done_result = co_await result.resolve_via(thread_executor, true);

            m_resuming_thread_id = thread::get_current_virtual_id();
            test_ready_result(std::move(done_result));
        }

       public:
        result<void> operator()(std::shared_ptr<manual_executor> manual_executor, std::shared_ptr<thread_executor> thread_executor) {
            assert_true(manual_executor->empty());

            auto result = inner_task(manual_executor, thread_executor);

            co_await thread_executor->submit([this, manual_executor]() mutable {
                m_setting_thread_id = thread::get_current_virtual_id();
                assert_true(manual_executor->loop_once());
            });

            co_await result;

            assert_not_equal(m_launcher_thread_id, m_setting_thread_id);
            assert_not_equal(m_launcher_thread_id, m_resuming_thread_id);
            assert_not_equal(m_setting_thread_id, m_resuming_thread_id);
        }
    };

    template<class type>
    struct test_resolve_via_not_ready_result<type, result_status::value, true> {
        // result is not ready (completes with a value) + executor throws
        // = resumed inline in the setting thread, errors::broken_task is thrown.

       private:
        uintptr_t m_launcher_thread_id = 0;
        uintptr_t m_setting_thread_id = 0;
        uintptr_t m_resuming_thread_id = 0;

        result<void> inner_task(std::shared_ptr<manual_executor> manual_executor,
                                std::shared_ptr<throwing_executor> throwing_executor) {
            m_launcher_thread_id = thread::get_current_virtual_id();

            auto result = manual_executor->submit([]() -> decltype(auto) {
                return value_gen<type>::default_value();
            });

            try {
                auto done_result = co_await result.resolve_via(throwing_executor, true);
            } catch (const errors::broken_task&) {
                m_resuming_thread_id = thread::get_current_virtual_id();
                co_return;
            } catch (...) {
            }

            assert_false(true);
        }

       public:
        result<void> operator()(std::shared_ptr<manual_executor> manual_executor,
                                std::shared_ptr<throwing_executor> throwing_executor,
                                std::shared_ptr<thread_executor> thread_executor) {
            assert_true(manual_executor->empty());

            auto result = inner_task(manual_executor, throwing_executor);

            co_await thread_executor->submit([this, manual_executor]() mutable {
                m_setting_thread_id = concurrencpp::details::thread::get_current_virtual_id();
                assert_true(manual_executor->loop_once());
            });

            co_await result;

            assert_not_equal(m_launcher_thread_id, m_setting_thread_id);
            assert_not_equal(m_launcher_thread_id, m_resuming_thread_id);
            assert_equal(m_setting_thread_id, m_resuming_thread_id);
        }
    };

    template<class type>
    struct test_resolve_via_not_ready_result<type, result_status::exception, false> {
        // result is not ready (completes with an exception) + executor doesn't throw
        // = rescheduling, asynchronous exception is returned

       private:
        uintptr_t m_launcher_thread_id = 0;
        uintptr_t m_setting_thread_id = 0;
        uintptr_t m_resuming_thread_id = 0;

        result<void> inner_task(std::shared_ptr<manual_executor> manual_executor, std::shared_ptr<thread_executor> thread_executor) {
            m_launcher_thread_id = thread::get_current_virtual_id();

            const size_t id = 1234567;
            auto result = manual_executor->submit([id]() -> decltype(auto) {
                throw custom_exception(id);
                return value_gen<type>::default_value();
            });

            auto done_result = co_await result.resolve_via(thread_executor, true);

            m_resuming_thread_id = thread::get_current_virtual_id();
            test_ready_result_custom_exception(std::move(done_result), id);
        }

       public:
        result<void> operator()(std::shared_ptr<manual_executor> manual_executor, std::shared_ptr<thread_executor> thread_executor) {
            assert_true(manual_executor->empty());

            auto result = inner_task(manual_executor, thread_executor);

            co_await thread_executor->submit([this, manual_executor]() mutable {
                m_setting_thread_id = concurrencpp::details::thread::get_current_virtual_id();
                assert_true(manual_executor->loop_once());
            });

            co_await result;

            assert_not_equal(m_launcher_thread_id, m_setting_thread_id);
            assert_not_equal(m_launcher_thread_id, m_resuming_thread_id);
            assert_not_equal(m_setting_thread_id, m_resuming_thread_id);
        }
    };

    template<class type>
    struct test_resolve_via_not_ready_result<type, result_status::exception, true> {
        // result is not ready (completes with an exception) + executor throws
        // = resumed inline in the setting thread, errors::broken_task is thrown.

       private:
        uintptr_t m_launcher_thread_id = 0;
        uintptr_t m_setting_thread_id = 0;
        uintptr_t m_resuming_thread_id = 0;

        result<void> inner_task(std::shared_ptr<manual_executor> manual_executor,
                                std::shared_ptr<throwing_executor> throwing_executor) {
            m_launcher_thread_id = thread::get_current_virtual_id();

            auto result = manual_executor->submit([]() -> decltype(auto) {
                return value_gen<type>::throw_ex();
            });

            try {
                co_await result.resolve_via(throwing_executor, true);
            } catch (const errors::broken_task&) {
                m_resuming_thread_id = thread::get_current_virtual_id();
                co_return;
            } catch (...) {
            }

            assert_false(true);
        }

       public:
        result<void> operator()(std::shared_ptr<manual_executor> manual_executor,
                                std::shared_ptr<throwing_executor> throwing_executor,
                                std::shared_ptr<thread_executor> thread_executor) {
            assert_true(manual_executor->empty());

            auto result = inner_task(manual_executor, throwing_executor);

            co_await thread_executor->submit([this, manual_executor]() mutable {
                m_setting_thread_id = thread::get_current_virtual_id();
                assert_true(manual_executor->loop_once());
            });

            co_await result;

            assert_not_equal(m_launcher_thread_id, m_setting_thread_id);
            assert_not_equal(m_launcher_thread_id, m_resuming_thread_id);
            assert_equal(m_setting_thread_id, m_resuming_thread_id);
        }
    };

}  // namespace concurrencpp::tests

template<class type>
void concurrencpp::tests::test_result_resolve_via_impl() {
    // empty result throws
    assert_throws_with_error_message<concurrencpp::errors::empty_result>(
        [] {
            auto executor = std::make_shared<inline_executor>();
            result<type>().resolve_via(executor);
        },
        concurrencpp::details::consts::k_result_resolve_via_error_msg);

    // null executor throws
    assert_throws_with_error_message<std::invalid_argument>(
        [] {
            auto result = result_gen<type>::ready();
            result.resolve_via({});
        },
        concurrencpp::details::consts::k_result_resolve_via_executor_null_error_msg);

    auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
    auto throwing_executor = std::make_shared<concurrencpp::tests::throwing_executor>();
    auto manual_executor = std::make_shared<concurrencpp::manual_executor>();

    executor_shutdowner es0(thread_executor), es1(throwing_executor), es2(manual_executor);

    test_resolve_via_ready_result<type, result_status::value, false, false>()(thread_executor).get();
    test_resolve_via_ready_result<type, result_status::value, false, true>()(throwing_executor).get();
    test_resolve_via_ready_result<type, result_status::value, true, false>()(thread_executor).get();
    test_resolve_via_ready_result<type, result_status::value, true, true>()(throwing_executor).get();

    test_resolve_via_ready_result<type, result_status::exception, false, false>()(thread_executor).get();
    test_resolve_via_ready_result<type, result_status::exception, false, true>()(throwing_executor).get();
    test_resolve_via_ready_result<type, result_status::exception, true, false>()(thread_executor).get();
    test_resolve_via_ready_result<type, result_status::exception, true, true>()(throwing_executor).get();

    test_resolve_via_not_ready_result<type, result_status::value, false>()(manual_executor, thread_executor).get();
    test_resolve_via_not_ready_result<type, result_status::value, true>()(manual_executor, throwing_executor, thread_executor).get();
    test_resolve_via_not_ready_result<type, result_status::exception, false>()(manual_executor, thread_executor).get();
    test_resolve_via_not_ready_result<type, result_status::exception, true>()(manual_executor, throwing_executor, thread_executor)
        .get();
}

void concurrencpp::tests::test_result_resolve_via() {
    test_result_resolve_via_impl<int>();
    test_result_resolve_via_impl<std::string>();
    test_result_resolve_via_impl<void>();
    test_result_resolve_via_impl<int&>();
    test_result_resolve_via_impl<std::string&>();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("result::resolve, result::resolve_via test");

    tester.add_step("reslove", test_result_resolve);
    tester.add_step("reslove_via", test_result_resolve_via);

    tester.launch_test();
    return 0;
}