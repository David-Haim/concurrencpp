#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/test_ready_result.h"
#include "utils/executor_shutdowner.h"

namespace concurrencpp::tests {
    template<class type>
    result<void> test_result_resolve_impl_result_ready_value();

    template<class type>
    result<void> test_result_resolve_impl_result_ready_exception();

    template<class type>
    result<void> test_result_resolve_impl_result_not_ready_value(std::shared_ptr<thread_executor> thread_executor);

    template<class type>
    result<void> test_result_resolve_impl_result_not_ready_exception(std::shared_ptr<thread_executor> thread_executor);

    template<class type>
    void test_result_resolve_impl();
    void test_result_resolve();

    template<class type>
    result<void> test_result_await_impl_result_ready_value();

    template<class type>
    result<void> test_result_await_impl_result_ready_exception();

    template<class type>
    result<void> test_result_await_impl_result_not_ready_value(std::shared_ptr<thread_executor> thread_executor);

    template<class type>
    result<void> test_result_await_impl_result_not_ready_exception(std::shared_ptr<thread_executor> thread_executor);

    template<class type>
    void test_result_await_impl();
    void test_result_await();

    template<class type>
    result<type> wrap_co_await(result<type> result) {
        co_return co_await result;
    }
}  // namespace concurrencpp::tests

using concurrencpp::result;
using concurrencpp::details::thread;

/*
    Test result statuses [ready. not ready] vs. result outcome [value, exception]
*/

template<class type>
result<void> concurrencpp::tests::test_result_resolve_impl_result_ready_value() {
    auto result = result_gen<type>::ready();

    const auto thread_id_0 = thread::get_current_virtual_id();

    auto done_result = co_await result.resolve();

    const auto thread_id_1 = thread::get_current_virtual_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    test_ready_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_impl_result_ready_exception() {
    const auto id = 1234567;
    auto result = make_exceptional_result<type>(custom_exception(id));

    const auto thread_id_0 = concurrencpp::details::thread::get_current_virtual_id();

    auto done_result = co_await result.resolve();

    const auto thread_id_1 = concurrencpp::details::thread::get_current_virtual_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    test_ready_result_custom_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_impl_result_not_ready_value(std::shared_ptr<thread_executor> thread_executor) {
    std::atomic_uintptr_t setting_thread_id = 0;

    result_promise<type> result_promise;
    auto result = result_promise.get_result();

    thread_executor->post([rp = std::move(result_promise), &setting_thread_id]() mutable {
        setting_thread_id = thread::get_current_virtual_id();
        rp.set_from_function([]() -> type {
            return value_gen<type>::default_value();
        });
    });

    auto done_result = co_await result.resolve();
    test_ready_result(std::move(done_result));
    assert_equal(thread::get_current_virtual_id(), setting_thread_id.load());
}

template<class type>
result<void> concurrencpp::tests::test_result_resolve_impl_result_not_ready_exception(
    std::shared_ptr<thread_executor> thread_executor) {
    std::atomic_uintptr_t setting_thread_id = 0;
    const auto id = 1234567;

    result_promise<type> result_promise;
    auto result = result_promise.get_result();

    thread_executor->post([rp = std::move(result_promise), id, &setting_thread_id]() mutable {
        setting_thread_id = thread::get_current_virtual_id();
        rp.set_exception(std::make_exception_ptr(custom_exception(id)));
    });

    auto done_result = co_await result.resolve();
    test_ready_result_custom_exception(std::move(done_result), id);
    assert_equal(thread::get_current_virtual_id(), setting_thread_id.load());
}

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
    executor_shutdowner es(thread_executor);

    test_result_resolve_impl_result_ready_value<type>().get();
    test_result_resolve_impl_result_ready_exception<type>().get();
    test_result_resolve_impl_result_not_ready_value<type>(thread_executor).get();
    test_result_resolve_impl_result_not_ready_exception<type>(thread_executor).get();
}

void concurrencpp::tests::test_result_resolve() {
    test_result_resolve_impl<int>();
    test_result_resolve_impl<std::string>();
    test_result_resolve_impl<void>();
    test_result_resolve_impl<int&>();
    test_result_resolve_impl<std::string&>();
}

template<class type>
result<void> concurrencpp::tests::test_result_await_impl_result_ready_value() {
    auto result = result_gen<type>::ready();

    const auto thread_id_0 = thread::get_current_virtual_id();

    auto done_result = co_await wrap_co_await(std::move(result)).resolve();

    const auto thread_id_1 = thread::get_current_virtual_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    test_ready_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_await_impl_result_ready_exception() {
    const auto id = 1234567;
    auto result = make_exceptional_result<type>(custom_exception(id));

    const auto thread_id_0 = concurrencpp::details::thread::get_current_virtual_id();

    auto done_result = co_await wrap_co_await(std::move(result)).resolve();

    const auto thread_id_1 = concurrencpp::details::thread::get_current_virtual_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    test_ready_result_custom_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::test_result_await_impl_result_not_ready_value(std::shared_ptr<thread_executor> thread_executor) {
    std::atomic_uintptr_t setting_thread_id = 0;

    result_promise<type> result_promise;
    auto result = result_promise.get_result();

    thread_executor->post([rp = std::move(result_promise), &setting_thread_id]() mutable {
        setting_thread_id = thread::get_current_virtual_id();
        rp.set_from_function([]() -> type {
            return value_gen<type>::default_value();
        });
    });

    auto done_result = co_await wrap_co_await(std::move(result)).resolve();
    test_ready_result(std::move(done_result));
    assert_equal(thread::get_current_virtual_id(), setting_thread_id.load());
}

template<class type>
result<void> concurrencpp::tests::test_result_await_impl_result_not_ready_exception(std::shared_ptr<thread_executor> thread_executor) {
    std::atomic_uintptr_t setting_thread_id = 0;
    const auto id = 1234567;

    result_promise<type> result_promise;
    auto result = result_promise.get_result();

    thread_executor->post([rp = std::move(result_promise), id, &setting_thread_id]() mutable {
        setting_thread_id = thread::get_current_virtual_id();
        rp.set_exception(std::make_exception_ptr(custom_exception(id)));
    });

    auto done_result = co_await wrap_co_await(std::move(result)).resolve();
    test_ready_result_custom_exception(std::move(done_result), id);
    assert_equal(thread::get_current_virtual_id(), setting_thread_id.load());
}

template<class type>
void concurrencpp::tests::test_result_await_impl() {
    // empty result throws
    {
        assert_throws_with_error_message<concurrencpp::errors::empty_result>(
            [] {
                result<type>().operator co_await();
            },
            concurrencpp::details::consts::k_result_operator_co_await_error_msg);
    }

    auto thread_executor = std::make_shared<concurrencpp::thread_executor>();
    executor_shutdowner es(thread_executor);

    test_result_await_impl_result_ready_value<type>().get();
    test_result_await_impl_result_ready_exception<type>().get();
    test_result_await_impl_result_not_ready_value<type>(thread_executor).get();
    test_result_await_impl_result_not_ready_exception<type>(thread_executor).get();
}

void concurrencpp::tests::test_result_await() {
    test_result_await_impl<int>();
    test_result_await_impl<std::string>();
    test_result_await_impl<void>();
    test_result_await_impl<int&>();
    test_result_await_impl<std::string&>();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("result::resolve + result::await");

    tester.add_step("resolve", test_result_resolve);
    tester.add_step("co_await", test_result_await);

    tester.launch_test();
    return 0;
}