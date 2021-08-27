#include "concurrencpp/concurrencpp.h"

#include "infra/tester.h"
#include "infra/assertions.h"
#include "utils/object_observer.h"
#include "utils/test_generators.h"
#include "utils/executor_shutdowner.h"
#include "utils/test_ready_result.h"
#include "utils/test_ready_lazy_result.h"

namespace concurrencpp::tests {
    template<class type>
    void test_lazy_result_constructor_impl();
    void test_lazy_result_constructor();

    template<class type>
    void test_lazy_result_destructor_impl();
    void test_lazy_result_destructor();

    template<class type>
    result<void> test_lazy_result_status_impl();
    void test_lazy_result_status();

    template<class type>
    void test_lazy_result_resolve_impl();
    void test_lazy_result_resolve();

    template<class type>
    void test_lazy_result_co_await_operator_impl();
    void test_lazy_result_co_await_operator();

    template<class type>
    void test_lazy_result_run_impl(std::shared_ptr<thread_executor> ex);
    void test_lazy_result_run();

    template<class type>
    void test_lazy_result_assignment_operator_self();

    template<class type>
    void test_lazy_result_assignment_operator_empty_to_empty();

    template<class type>
    void test_lazy_result_assignment_operator_non_empty_to_empty();

    template<class type>
    void test_lazy_result_assignment_operator_empty_to_non_empty();

    template<class type>
    void test_lazy_result_assignment_operator_non_empty_to_non_empty();

    template<class type>
    void test_lazy_result_assignment_operator_impl();

    void test_lazy_result_assignment_operator();
}  // namespace concurrencpp::tests

namespace concurrencpp::tests {
    template<class type>
    lazy_result<type> sync_lazy_coro() {
        co_return value_gen<type>::default_value();
    }

    template<class type>
    lazy_result<type> sync_lazy_coro(testing_stub stub) {
        co_return value_gen<type>::default_value();
    }

    template<class type>
    lazy_result<type> sync_lazy_coro(testing_stub stub, value_gen<type>& gen, size_t i) {
        co_return gen.value_of(i);
    }

    template<class type>
    lazy_result<type> sync_lazy_coro_ex(intptr_t id) {
        throw custom_exception(id);
        co_return value_gen<type>::default_value();
    }

    template<class type>
    lazy_result<type> async_lazy_coro_val(bool& started, std::shared_ptr<thread_executor> ex) {
        started = true;
        co_await resume_on(ex);
        co_return value_gen<type>::default_value();
    }

    template<class type>
    lazy_result<type> async_lazy_coro_ex(bool& started, std::shared_ptr<thread_executor> ex, intptr_t id) {
        started = true;
        co_await resume_on(ex);
        throw custom_exception(id);
        co_return value_gen<type>::default_value();
    }
}  // namespace concurrencpp::tests

template<class type>
void concurrencpp::tests::test_lazy_result_constructor_impl() {
    // default ctor
    {
        lazy_result<type> result;
        assert_false(static_cast<bool>(result));
    }

    // move ctor
    {
        lazy_result<type> result = sync_lazy_coro<type>();
        assert_true(static_cast<bool>(result));

        auto other = std::move(result);
        assert_true(static_cast<bool>(other));
        assert_false(static_cast<bool>(result));
    }
}

void concurrencpp::tests::test_lazy_result_constructor() {
    test_lazy_result_constructor_impl<int>();
    test_lazy_result_constructor_impl<std::string>();
    test_lazy_result_constructor_impl<void>();
    test_lazy_result_constructor_impl<int&>();
    test_lazy_result_constructor_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_lazy_result_destructor_impl() {
    object_observer observer;

    {
        auto result = sync_lazy_coro<type>(observer.get_testing_stub());
        assert_equal(observer.get_destruction_count(), 0);
    }

    assert_equal(observer.get_destruction_count(), 1);
}

void concurrencpp::tests::test_lazy_result_destructor() {
    test_lazy_result_destructor_impl<int>();
    test_lazy_result_destructor_impl<std::string>();
    test_lazy_result_destructor_impl<void>();
    test_lazy_result_destructor_impl<int&>();
    test_lazy_result_destructor_impl<std::string&>();
}

template<class type>
concurrencpp::result<void> concurrencpp::tests::test_lazy_result_status_impl() {
    assert_throws_with_error_message<errors::empty_result>(
        [] {
            lazy_result<type>().status();
        },
        concurrencpp::details::consts::k_empty_lazy_result_status_err_msg);

    // value
    {
        lazy_result<type> result = sync_lazy_coro<type>();
        assert_equal(result.status(), result_status::idle);

        const auto done_result = co_await result.resolve();
        assert_equal(done_result.status(), result_status::value);
    }

    // exception
    {
        lazy_result<type> result = sync_lazy_coro_ex<type>(12345);
        assert_equal(result.status(), result_status::idle);

        const auto done_result = co_await result.resolve();
        assert_equal(done_result.status(), result_status::exception);
    }
}

void concurrencpp::tests::test_lazy_result_status() {
    test_lazy_result_status_impl<int>().get();
    test_lazy_result_status_impl<std::string>().get();
    test_lazy_result_status_impl<void>().get();
    test_lazy_result_status_impl<int&>().get();
    test_lazy_result_status_impl<std::string&>().get();
}

using concurrencpp::details::thread;

namespace concurrencpp::tests {
    template<class type>
    result<void> test_lazy_result_resolve_non_ready_coro_val(std::shared_ptr<thread_executor> ex) {
        const auto thread_id_before = thread::get_current_virtual_id();
        auto started = false;
        auto result = async_lazy_coro_val<type>(started, ex);

        assert_false(started);
        assert_equal(result.status(), result_status::idle);

        auto done_result = co_await result.resolve();
        const auto thread_id_after = thread::get_current_virtual_id();

        test_ready_lazy_result(std::move(done_result));
        assert_not_equal(thread_id_before, thread_id_after);
    }

    template<class type>
    result<void> test_lazy_result_resolve_non_ready_coro_ex(std::shared_ptr<thread_executor> ex) {
        const auto thread_id_before = thread::get_current_virtual_id();
        constexpr intptr_t id = 987654321;

        auto started = false;
        auto result = async_lazy_coro_ex<type>(started, ex, id);

        assert_false(started);
        assert_equal(result.status(), result_status::idle);

        auto done_result = co_await result.resolve();
        const auto thread_id_after = thread::get_current_virtual_id();

        test_ready_lazy_result_custom_exception(std::move(done_result), id);
        assert_not_equal(thread_id_before, thread_id_after);
    }

    template<class type>
    result<void> test_lazy_result_resolve_ready_coro_val() {
        auto done_result = co_await sync_lazy_coro<type>().resolve();

        const auto thread_id_before = thread::get_current_virtual_id();

        auto result = co_await done_result.resolve();

        const auto thread_id_after = thread::get_current_virtual_id();

        test_ready_lazy_result(std::move(result));
        assert_equal(thread_id_before, thread_id_after);
    }

    template<class type>
    result<void> test_lazy_result_resolve_ready_coro_ex() {
        constexpr intptr_t id = 987654321;
        auto done_result = co_await sync_lazy_coro_ex<type>(id).resolve();

        const auto thread_id_before = thread::get_current_virtual_id();

        auto result = co_await done_result.resolve();

        const auto thread_id_after = thread::get_current_virtual_id();

        test_ready_lazy_result_custom_exception(std::move(result), id);
        assert_equal(thread_id_before, thread_id_after);
    }
}  // namespace concurrencpp::tests

template<class type>
void concurrencpp::tests::test_lazy_result_resolve_impl() {
    assert_throws_with_error_message<errors::empty_result>(
        [] {
            lazy_result<type>().resolve();
        },
        concurrencpp::details::consts::k_empty_lazy_result_resolve_err_msg);

    auto ex = std::make_shared<thread_executor>();
    executor_shutdowner es(ex);

    test_lazy_result_resolve_non_ready_coro_val<type>(ex).get();
    test_lazy_result_resolve_non_ready_coro_ex<type>(ex).get();
    test_lazy_result_resolve_ready_coro_val<type>().get();
    test_lazy_result_resolve_ready_coro_ex<type>().get();
}

void concurrencpp::tests::test_lazy_result_resolve() {
    test_lazy_result_resolve_impl<int>();
    test_lazy_result_resolve_impl<std::string>();
    test_lazy_result_resolve_impl<void>();
    test_lazy_result_resolve_impl<int&>();
    test_lazy_result_resolve_impl<std::string&>();
}

namespace concurrencpp::tests {
    template<class type>
    lazy_result<type> proxy_coro(lazy_result<type> result) {
        co_return co_await result;
    }

    template<class type>
    result<void> test_lazy_result_co_await_non_ready_coro_val(std::shared_ptr<thread_executor> ex) {
        const auto thread_id_before = thread::get_current_virtual_id();
        auto started = false;
        auto result = async_lazy_coro_val<type>(started, ex);
        auto proxy_result = proxy_coro(std::move(result));

        assert_false(started);
        assert_equal(proxy_result.status(), result_status::idle);

        auto done_result = co_await proxy_result.resolve();
        const auto thread_id_after = thread::get_current_virtual_id();

        assert_true(started);
        test_ready_lazy_result(std::move(done_result));
        assert_not_equal(thread_id_before, thread_id_after);
    }

    template<class type>
    result<void> test_lazy_result_co_await_non_ready_coro_ex(std::shared_ptr<thread_executor> ex) {
        constexpr intptr_t id = 987654321;
        auto started = false;
        const auto thread_id_before = thread::get_current_virtual_id();

        auto result = async_lazy_coro_ex<type>(started, ex, id);
        auto proxy_result = proxy_coro(std::move(result));

        assert_false(started);

        assert_equal(proxy_result.status(), result_status::idle);

        auto done_result = co_await proxy_result.resolve();

        const auto thread_id_after = thread::get_current_virtual_id();

        assert_true(started);
        test_ready_lazy_result_custom_exception(std::move(done_result), id);
        assert_not_equal(thread_id_before, thread_id_after);
    }

    template<class type>
    result<void> test_lazy_result_co_await_ready_coro_val() {
        auto done_result = co_await proxy_coro(sync_lazy_coro<type>()).resolve();

        const auto thread_id_before = thread::get_current_virtual_id();

        auto result = co_await done_result.resolve();

        const auto thread_id_after = thread::get_current_virtual_id();

        test_ready_lazy_result(std::move(result));
        assert_equal(thread_id_before, thread_id_after);
    }

    template<class type>
    result<void> test_lazy_result_co_await_ready_coro_ex() {
        constexpr intptr_t id = 987654321;
        auto done_result = co_await proxy_coro(sync_lazy_coro_ex<type>(id)).resolve();

        const auto thread_id_before = thread::get_current_virtual_id();

        auto result = co_await done_result.resolve();

        const auto thread_id_after = thread::get_current_virtual_id();

        test_ready_lazy_result_custom_exception(std::move(result), 987654321);
        assert_equal(thread_id_before, thread_id_after);
    }
}  // namespace concurrencpp::tests

template<class type>
void concurrencpp::tests::test_lazy_result_co_await_operator_impl() {
    assert_throws_with_error_message<errors::empty_result>(
        [] {
            lazy_result<type>().operator co_await();
        },
        concurrencpp::details::consts::k_empty_lazy_result_operator_co_await_err_msg);

    runtime runtime;
    test_lazy_result_co_await_non_ready_coro_val<type>(runtime.thread_executor()).get();
    test_lazy_result_co_await_non_ready_coro_ex<type>(runtime.thread_executor()).get();
    test_lazy_result_co_await_ready_coro_val<type>().get();
    test_lazy_result_co_await_ready_coro_ex<type>().get();
}

void concurrencpp::tests::test_lazy_result_co_await_operator() {
    test_lazy_result_co_await_operator_impl<int>();
    test_lazy_result_co_await_operator_impl<std::string>();
    test_lazy_result_co_await_operator_impl<void>();
    test_lazy_result_co_await_operator_impl<int&>();
    test_lazy_result_co_await_operator_impl<std::string&>();
}

template<class type>
void concurrencpp::tests::test_lazy_result_run_impl(std::shared_ptr<thread_executor> ex) {
    assert_throws_with_error_message<errors::empty_result>(
        [] {
            lazy_result<type>().run();
        },
        concurrencpp::details::consts::k_empty_lazy_result_run_err_msg);

    auto started = false;
    auto lazy = async_lazy_coro_val<type>(started, ex);

    assert_false(started);

    auto result = lazy.run();

    assert_true(started);
    assert_false(static_cast<bool>(lazy));
    assert_true(static_cast<bool>(result));

    result.wait();

    test_ready_result(std::move(result));
}

void concurrencpp::tests::test_lazy_result_run() {
    runtime runtime;
    test_lazy_result_run_impl<int>(runtime.thread_executor());
    test_lazy_result_run_impl<std::string>(runtime.thread_executor());
    test_lazy_result_run_impl<void>(runtime.thread_executor());
    test_lazy_result_run_impl<int&>(runtime.thread_executor());
    test_lazy_result_run_impl<std::string&>(runtime.thread_executor());
}

template<class type>
void concurrencpp::tests::test_lazy_result_assignment_operator_self() {
    object_observer observer;

    {
        auto res = sync_lazy_coro<type>(observer.get_testing_stub());
        assert_true(static_cast<bool>(res));

        res = std::move(res);
        assert_true(static_cast<bool>(res));
        res.run().get();
    }

    assert_equal(observer.get_destruction_count(), 1);
}

template<class type>
void concurrencpp::tests::test_lazy_result_assignment_operator_empty_to_empty() {
    lazy_result<type> res0, res1;
    res0 = std::move(res1);

    assert_false(static_cast<bool>(res0));
    assert_false(static_cast<bool>(res1));
}

template<class type>
void concurrencpp::tests::test_lazy_result_assignment_operator_non_empty_to_empty() {
    object_observer observer;
    value_gen<type> gen;

    lazy_result<type> res0, res1 = sync_lazy_coro<type>(observer.get_testing_stub(), gen, 1);
    res0 = std::move(res1);

    assert_true(static_cast<bool>(res0));
    assert_false(static_cast<bool>(res1));

    assert_equal(observer.get_destruction_count(), 0);

    if constexpr (!std::is_same_v<void, type>) {
        assert_equal(res0.run().get(), gen.value_of(1));
    }
}

template<class type>
void concurrencpp::tests::test_lazy_result_assignment_operator_empty_to_non_empty() {
    object_observer observer;

    lazy_result<type> res0, res1 = sync_lazy_coro<type>(observer.get_testing_stub());
    res1 = std::move(res0);

    assert_false(static_cast<bool>(res0));
    assert_false(static_cast<bool>(res1));

    assert_equal(observer.get_destruction_count(), 1);
}

template<class type>
void concurrencpp::tests::test_lazy_result_assignment_operator_non_empty_to_non_empty() {
    object_observer observer;
    value_gen<type> gen;

    lazy_result<type> res0 = sync_lazy_coro<type>(observer.get_testing_stub(), gen, 0);
    lazy_result<type> res1 = sync_lazy_coro<type>(observer.get_testing_stub(), gen, 1);
    res0 = std::move(res1);

    assert_true(static_cast<bool>(res0));
    assert_false(static_cast<bool>(res1));

    assert_equal(observer.get_destruction_count(), 1);
    if constexpr (!std::is_same_v<void, type>) {
        assert_equal(res0.run().get(), gen.value_of(1));
    }
}

template<class type>
void concurrencpp::tests::test_lazy_result_assignment_operator_impl() {
    test_lazy_result_assignment_operator_self<type>();
    test_lazy_result_assignment_operator_empty_to_empty<type>();
    test_lazy_result_assignment_operator_non_empty_to_empty<type>();
    test_lazy_result_assignment_operator_empty_to_non_empty<type>();
    test_lazy_result_assignment_operator_non_empty_to_non_empty<type>();
}

void concurrencpp::tests::test_lazy_result_assignment_operator() {
    test_lazy_result_assignment_operator_impl<int>();
    test_lazy_result_assignment_operator_impl<std::string>();
    test_lazy_result_assignment_operator_impl<void>();
    test_lazy_result_assignment_operator_impl<int&>();
    test_lazy_result_assignment_operator_impl<std::string&>();
}

using namespace concurrencpp::tests;

int main() {
    tester tester("lazy_result test");

    tester.add_step("constructor", test_lazy_result_constructor);
    tester.add_step("destructor", test_lazy_result_destructor);
    tester.add_step("status", test_lazy_result_status);
    tester.add_step("resolve", test_lazy_result_resolve);
    tester.add_step("operator co_await", test_lazy_result_co_await_operator);
    tester.add_step("run", test_lazy_result_run);
    tester.add_step("operator =", test_lazy_result_assignment_operator);

    tester.launch_test();
    return 0;
}