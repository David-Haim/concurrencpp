#include "concurrencpp/concurrencpp.h"
#include "tests/all_tests.h"

#include "tests/test_utils/result_factory.h"
#include "tests/test_utils/test_ready_result.h"
#include "tests/test_utils/test_executors.h"
#include "tests/test_utils/executor_shutdowner.h"
#include "tests/test_utils/proxy_coro.h"

#include "tester/tester.h"
#include "helpers/assertions.h"
#include "helpers/random.h"

namespace concurrencpp::tests {
template<class type>
result<void> test_result_await_ready_val();

template<class type>
result<void> test_result_await_ready_err();

template<class type>
result<void> test_result_await_not_ready_val(
    std::shared_ptr<test_executor> executor);

template<class type>
result<void> test_result_await_not_ready_err(
    std::shared_ptr<test_executor> executor);

template<class type>
void test_result_await_impl();
void test_result_await();

template<class type>
result<void> test_result_await_via_ready_val(
    std::shared_ptr<test_executor> executor);

template<class type>
result<void> test_result_await_via_ready_err(
    std::shared_ptr<test_executor> executor);

template<class type>
result<void> test_result_await_via_ready_val_force_rescheduling(
    std::shared_ptr<test_executor> executor);

template<class type>
result<void> test_result_await_via_ready_err_force_rescheduling(
    std::shared_ptr<test_executor> executor);

template<class type>
result<void>
test_result_await_via_ready_val_force_rescheduling_executor_threw();

template<class type>
result<void>
test_result_await_via_ready_err_force_rescheduling_executor_threw();

template<class type>
result<void> test_result_await_via_not_ready_val(
    std::shared_ptr<test_executor> executor);
template<class type>
result<void> test_result_await_via_not_ready_err(
    std::shared_ptr<test_executor> executor);

template<class type>
result<void> test_result_await_via_not_ready_val_executor_threw(
    std::shared_ptr<test_executor> executor);
template<class type>
result<void> test_result_await_via_not_ready_err_executor_threw(
    std::shared_ptr<test_executor> executor);

template<class type>
void test_result_await_via_impl();
void test_result_await_via();
}  // namespace concurrencpp::tests

using concurrencpp::result;
using concurrencpp::result_promise;
using namespace std::chrono;

/*
        At this point, we know result::resolve(_via) works perfectly so we wrap
   co_await operator in another resolving-coroutine and we continue testing it
   as a resolve test. co_await will do its thing and it'll forward the
   result/exception to a resolving coroutine. since result::resolve(_via) works
   with no bugs, every failure is caused by result::co_await
*/

template<class type>
result<void> concurrencpp::tests::test_result_await_ready_val() {
    auto result = result_factory<type>::make_ready();
    const auto thread_id_0 = std::this_thread::get_id();

    auto result_proxy =
        [result = std::move(result)]() mutable -> concurrencpp::result<type> {
        co_return co_await result;
    };

    auto done_result = co_await result_proxy().resolve();

    const auto thread_id_1 = std::this_thread::get_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_await_ready_err() {
    random randomizer;
    const auto id = randomizer();
    auto result = make_exceptional_result<type>(costume_exception(id));

    const auto thread_id_0 = std::this_thread::get_id();

    auto result_proxy =
        [result = std::move(result)]() mutable -> concurrencpp::result<type> {
        co_return co_await result;
    };

    auto done_result = co_await result_proxy().resolve();

    const auto thread_id_1 = std::this_thread::get_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::test_result_await_not_ready_val(
    std::shared_ptr<test_executor> executor) {
    result_promise<type> rp;
    auto result = rp.get_result();

    executor->set_rp_value(std::move(rp));

    auto result_proxy =
        [result = std::move(result)]() mutable -> concurrencpp::result<type> {
        co_return co_await result;
    };

    auto done_result = co_await result_proxy().resolve();

    assert_false(static_cast<bool>(result));
    assert_true(
        executor->scheduled_inline());  // the thread that set the value is the
    // thread that resumes the coroutine.
    test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_await_not_ready_err(
    std::shared_ptr<test_executor> executor) {
    result_promise<type> rp;
    auto result = rp.get_result();

    const auto id = executor->set_rp_err(std::move(rp));

    auto result_proxy =
        [result = std::move(result)]() mutable -> concurrencpp::result<type> {
        co_return co_await result;
    };

    auto done_result = co_await result_proxy().resolve();

    assert_false(static_cast<bool>(result));
    assert_true(
        executor->scheduled_inline());  // the thread that set the value is the
    // thread that resumes the coroutine.
    test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
void concurrencpp::tests::test_result_await_impl() {
    // empty result throws
    {
        assert_throws<concurrencpp::errors::empty_result>([] {
            result<type> result;
            result.operator co_await();
        });
    }

    // ready result resumes immediately in the awaiting thread with no
    // rescheduling
    test_result_await_ready_val<type>().get();

    // ready result resumes immediately in the awaiting thread with no
    // rescheduling
    test_result_await_ready_err<type>().get();

    // if value or exception are not available - suspend and resume in the setting
    // thread
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_not_ready_val<type>(te).get();
    }

    // if value or exception are not available - suspend and resume in the setting
    // thread
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_not_ready_err<type>(te).get();
    }
}

void concurrencpp::tests::test_result_await() {
    test_result_await_impl<int>();
    test_result_await_impl<std::string>();
    test_result_await_impl<void>();
    test_result_await_impl<int&>();
    test_result_await_impl<std::string&>();
}

template<class type>
result<void> concurrencpp::tests::test_result_await_via_ready_val(
    std::shared_ptr<test_executor> executor) {
    auto result = result_factory<type>::make_ready();

    const auto thread_id_0 = std::this_thread::get_id();

    proxy_coro coro(std::move(result), executor, false);
    auto done_result = co_await coro().resolve();

    const auto thread_id_1 = std::this_thread::get_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    assert_false(executor->scheduled_async());
    test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_await_via_ready_err(
    std::shared_ptr<test_executor> executor) {
    random randomizer;
    const auto id = randomizer();
    auto result = make_exceptional_result<type>(costume_exception(id));

    const auto thread_id_0 = std::this_thread::get_id();

    proxy_coro coro(std::move(result), executor, false);
    auto done_result = co_await coro().resolve();

    const auto thread_id_1 = std::this_thread::get_id();

    assert_false(static_cast<bool>(result));
    assert_equal(thread_id_0, thread_id_1);
    assert_false(executor->scheduled_async());
    test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void>
concurrencpp::tests::test_result_await_via_ready_val_force_rescheduling(
    std::shared_ptr<test_executor> executor) {
    auto result = result_factory<type>::make_ready();

    proxy_coro coro(std::move(result), executor, true);
    auto done_result = co_await coro().resolve();

    assert_false(executor->scheduled_inline());
    assert_true(executor->scheduled_async());
    test_ready_result_result(std::move(done_result));
}

template<class type>
result<void>
concurrencpp::tests::test_result_await_via_ready_err_force_rescheduling(
    std::shared_ptr<test_executor> executor) {
    random randomizer;
    auto id = static_cast<size_t>(randomizer());
    auto result = make_exceptional_result<type>(costume_exception(id));

    proxy_coro coro(std::move(result), executor, true);
    auto done_result = co_await coro().resolve();

    assert_false(static_cast<bool>(result));
    assert_false(executor->scheduled_inline());
    assert_true(executor->scheduled_async());
    test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void> concurrencpp::tests::
    test_result_await_via_ready_val_force_rescheduling_executor_threw() {
    auto result = result_factory<type>::make_ready();
    auto te = std::make_shared<throwing_executor>();

    auto thread_id_before = std::this_thread::get_id();

    try {
        proxy_coro coro(std::move(result), te, true);
        auto done_result = co_await coro().resolve();
        co_await done_result;
    } catch (const executor_enqueue_exception&) {
        // do nothing
    } catch (...) {
        assert_false(true);
    }

    auto thread_id_after = std::this_thread::get_id();

    assert_equal(thread_id_before, thread_id_after);
    assert_false(static_cast<bool>(result));
}

template<class type>
result<void> concurrencpp::tests::
    test_result_await_via_ready_err_force_rescheduling_executor_threw() {
    auto result = result_factory<type>::make_exceptional();
    auto te = std::make_shared<throwing_executor>();

    auto thread_id_before = std::this_thread::get_id();

    try {
        proxy_coro coro(std::move(result), te, true);
        auto done_result = co_await coro().resolve();
        assert_true(static_cast<bool>(done_result));

        co_await done_result;
    } catch (const executor_enqueue_exception&) {
        // do nothing
    } catch (...) {
        assert_false(true);
    }

    auto thread_id_after = std::this_thread::get_id();

    assert_equal(thread_id_before, thread_id_after);
    assert_false(static_cast<bool>(result));
}

template<class type>
result<void> concurrencpp::tests::test_result_await_via_not_ready_val(
    std::shared_ptr<test_executor> executor) {
    result_promise<type> rp;
    auto result = rp.get_result();

    executor->set_rp_value(std::move(rp));

    proxy_coro coro(std::move(result), executor, true);
    auto done_result = co_await coro().resolve();

    assert_false(static_cast<bool>(result));
    assert_false(executor->scheduled_inline());
    assert_true(executor->scheduled_async());
    test_ready_result_result(std::move(done_result));
}

template<class type>
result<void> concurrencpp::tests::test_result_await_via_not_ready_err(
    std::shared_ptr<test_executor> executor) {
    result_promise<type> rp;
    auto result = rp.get_result();

    const auto id = executor->set_rp_err(std::move(rp));

    proxy_coro coro(std::move(result), executor, true);
    auto done_result = co_await coro().resolve();

    assert_false(static_cast<bool>(result));
    assert_true(executor->scheduled_async());
    assert_false(executor->scheduled_inline());
    test_ready_result_costume_exception(std::move(done_result), id);
}

template<class type>
result<void>
concurrencpp::tests::test_result_await_via_not_ready_val_executor_threw(
    std::shared_ptr<test_executor> executor) {
    result_promise<type> rp;
    auto result = rp.get_result();
    auto te = std::make_shared<throwing_executor>();

    executor->set_rp_value(std::move(rp));

    proxy_coro coro(std::move(result), te, true);
    auto done_result = co_await coro().resolve();

    assert_false(static_cast<bool>(result));
    assert_true(executor->scheduled_inline());  // since te threw, execution is
        // resumed in ex::m_setting_thread
    test_executor_error_thrown(std::move(done_result), te);
}

template<class type>
result<void>
concurrencpp::tests::test_result_await_via_not_ready_err_executor_threw(
    std::shared_ptr<test_executor> executor) {
    result_promise<type> rp;
    auto result = rp.get_result();

    auto te = std::make_shared<throwing_executor>();

    const auto id = executor->set_rp_err(std::move(rp));
    (void)id;

    proxy_coro coro(std::move(result), te, true);
    auto done_result = co_await coro().resolve();

    assert_false(static_cast<bool>(result));
    assert_true(executor->scheduled_inline());  // since te threw, execution is
        // resumed in ex::m_setting_thread
    test_executor_error_thrown(std::move(done_result), te);
}

template<class type>
void concurrencpp::tests::test_result_await_via_impl() {
    // empty result throws
    assert_throws<concurrencpp::errors::empty_result>([] {
        result<type> result;
        auto executor = make_test_executor();
        result.await_via(executor);
    });

    assert_throws_with_error_message<std::invalid_argument>(
        [] {
            auto result = result_factory<type>::make_ready();
            result.await_via({}, true);
        },
        concurrencpp::details::consts::
            k_result_await_via_executor_null_error_msg);

    // if the result is ready by value, and force_rescheduling = false, resume in
    // the calling thread
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_ready_val<type>(te).get();
    }

    // if the result is ready by exception, and force_rescheduling = false, resume
    // in the calling thread
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_ready_err<type>(te).get();
    }

    // if the result is ready by value, and force_rescheduling = true, forcefully
    // resume execution through the executor
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_ready_val_force_rescheduling<type>(te).get();
    }

    // if the result is ready by exception, and force_rescheduling = true,
    // forcefully resume execution through the executor
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_ready_err_force_rescheduling<type>(te).get();
    }

    // if execution is rescheduled by a throwing executor, reschdule inline and
    // throw executor_exception
    test_result_await_via_ready_val_force_rescheduling_executor_threw<type>()
        .get();

    // if execution is rescheduled by a throwing executor, reschdule inline and
    // throw executor_exception
    test_result_await_via_ready_err_force_rescheduling_executor_threw<type>()
        .get();

    // if result is not ready - the execution resumes through the executor
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_not_ready_val<type>(te).get();
    }

    // if result is not ready - the execution resumes through the executor
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_not_ready_err<type>(te).get();
    }

    // if result is not ready - the execution resumes through the executor
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_not_ready_val_executor_threw<type>(te).get();
    }

    // if result is not ready - the execution resumes through the executor
    {
        auto te = make_test_executor();
        executor_shutdowner es(te);
        test_result_await_via_not_ready_err_executor_threw<type>(te).get();
    }
}

void concurrencpp::tests::test_result_await_via() {
    test_result_await_via_impl<int>();
    test_result_await_via_impl<std::string>();
    test_result_await_via_impl<void>();
    test_result_await_via_impl<int&>();
    test_result_await_via_impl<std::string&>();
}

void concurrencpp::tests::test_result_await_all() {
    tester tester("result::await, result::await_via test");

    tester.add_step("await", test_result_await);
    tester.add_step("await_via", test_result_await_via);

    tester.launch_test();
}
